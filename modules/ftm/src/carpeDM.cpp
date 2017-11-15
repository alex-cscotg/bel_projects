#include <boost/shared_ptr.hpp>
#include <algorithm>  
#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <boost/graph/graphviz.hpp>
#include <boost/algorithm/string.hpp>

#include "common.h"
#include "propwrite.h"
#include "graph.h"
#include "carpeDM.h"
#include "minicommand.h"
#include "dotstr.h"
#include "idformat.h"

  namespace dgp = DotStr::Graph::Prop;
  namespace dnp = DotStr::Node::Prop;
  namespace dep = DotStr::Edge::Prop;



int CarpeDM::ebWriteCycle(Device& dev, vAdr va, vBuf& vb)
{
   //eb_status_t status;
   //FIXME What about MTU? What about returned eb status ??
   Cycle cyc;
   ebBuf veb = ebBuf(va.size());

   for(int i = 0; i < (va.end()-va.begin()); i++) {
     uint32_t data = vb[i*4 + 0] << 24 | vb[i*4 + 1] << 16 | vb[i*4 + 2] << 8 | vb[i*4 + 3];
     veb[i] = data;
   } 

   cyc.open(dev);
   for(int i = 0; i < (veb.end()-veb.begin()); i++) {
    //FIXME dirty break into cycles
    if (i && ((va[i] & (RAM_SIZE-1)) ^ (va[i-1] & (RAM_SIZE-1)))) {
      cyc.close();
      cyc.open(dev);  
    }
    std::cout << "Writing @ 0x" << std::hex << std::setfill('0') << std::setw(8) << va[i] << " : 0x" << std::hex << std::setfill('0') << std::setw(8) << veb[i] << std::endl;
    cyc.write(va[i], EB_BIG_ENDIAN | EB_DATA32, (eb_data_t)veb[i]);

   }
   cyc.close();
   
   return 0;
}

vBuf CarpeDM::ebReadCycle(Device& dev, vAdr va)
{
   //FIXME What about MTU? What about returned eb status ??
   Cycle cyc;
   uint32_t veb[va.size()];
   vBuf ret   = vBuf(va.size() * 4);
      
   //sLog << "Got Adr Vec with " << va.size() << " Adrs" << std::endl;

   cyc.open(dev);
   for(int i = 0; i < (va.end()-va.begin()); i++) {
    //FIXME dirty break into cycles
    if (i && ((va[i] & (RAM_SIZE-1)) ^ (va[i-1] & (RAM_SIZE-1)))) {
      cyc.close();
      cyc.open(dev);  
    }
    cyc.read(va[i], EB_BIG_ENDIAN | EB_DATA32, (eb_data_t*)&veb[i]);
   }
   cyc.close();

  for(unsigned int i = 0; i < va.size(); i++) { 
    ret[i * 4 + 0] = (uint8_t)(veb[i] >> 24);
    ret[i * 4 + 1] = (uint8_t)(veb[i] >> 16);
    ret[i * 4 + 2] = (uint8_t)(veb[i] >> 8);
    ret[i * 4 + 3] = (uint8_t)(veb[i] >> 0);
  } 

  return ret;
}

int CarpeDM::ebWriteWord(Device& dev, uint32_t adr, uint32_t data)
{
   Cycle cyc;
   //FIXME What about returned eb status ??
   std::cout << "Writing @ 0x" << std::hex << std::setfill('0') << std::setw(8) << adr << " : 0x" << std::hex << std::setfill('0') << std::setw(8) << data << std::endl;
   cyc.open(dev);
   cyc.write(adr, EB_BIG_ENDIAN | EB_DATA32, (eb_data_t)data);
   cyc.close();
   
   return 0;
}

uint32_t CarpeDM::ebReadWord(Device& dev, uint32_t adr)
{
   //FIXME this sometimes led to memory corruption by eb's handling of &data - investigate !!!
   uint32_t data, ret;

   Cycle cyc;
   cyc.open(dev);
   cyc.read(adr, EB_BIG_ENDIAN | EB_DATA32, (eb_data_t*)&data);
   cyc.close();
   
   ret = data;

   return ret;
}

 //Reads and returns a 64 bit word from DM
uint64_t CarpeDM::read64b(uint32_t startAdr) {
  vAdr vA({startAdr + 0, startAdr + _32b_SIZE_});
  vBuf vD = ebReadCycle(ebd, vA);
  uint8_t* b = &vD[0];

  return writeBeBytesToLeNumber<uint64_t>(b); 
}

int CarpeDM::write64b(uint32_t startAdr, uint64_t d) {
  uint8_t b[_TS_SIZE_];
  writeLeNumberToBeBytes(b, d);
  vAdr vA({startAdr + 0, startAdr + _32b_SIZE_});
  vBuf vD(std::begin(b), std::end(b) );

  return ebWriteCycle(ebd, vA, vD);

}


bool CarpeDM::connect(const std::string& en) {
    ebdevname = std::string(en); //copy to avoid mem trouble later
    bool  ret = false;
    uint8_t mappedIdx = 0;
    int expVersion = parseFwVersionString(EXP_VER), foundVersion;

    if (expVersion <= 0) {throw std::runtime_error("Bad required minimum firmware version string received from Makefile"); return false;}

    atUp.clear();
    atUp.removeMemories();
    gUp.clear();
    atDown.clear();
    atDown.removeMemories();
    gDown.clear();
    cpuIdxMap.clear();
    vFw.clear();

    if(verbose) sLog << "Connecting to " << en << "... ";
    try { 
      ebs.open(0, EB_DATAX|EB_ADDRX);
      ebd.open(ebs, ebdevname.c_str(), EB_DATAX|EB_ADDRX, 3);
      ebd.sdb_find_by_identity(SDB_VENDOR_GSI,SDB_DEVICE_LM32_RAM, myDevs);
      if (myDevs.size() >= 1) { 
        cpuQty = myDevs.size();

        for(int cpuIdx = 0; cpuIdx< cpuQty; cpuIdx++) {
          //only create MemUnits for valid DM CPUs, generate Mapping so we can still use the cpuIdx supplied by User 
          foundVersion = getFwVersion(cpuIdx);

          vFw.push_back(foundVersion);
          if (expVersion <= foundVersion) {
            cpuIdxMap[cpuIdx]    = mappedIdx;
            uint32_t extBaseAdr   = myDevs[cpuIdx].sdb_component.addr_first;
            uint32_t intBaseAdr   = getIntBaseAdr(cpuIdx);
            uint32_t peerBaseAdr  = WORLD_BASE_ADR  + extBaseAdr; 
            uint32_t sharedOffs   = getSharedOffs(cpuIdx) + _SHCTL_END_; 
            uint32_t space        = getSharedSize(cpuIdx) - _SHCTL_END_;
                        
              atUp.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space );
            atDown.addMemory(cpuIdx, extBaseAdr, intBaseAdr, peerBaseAdr, sharedOffs, space );
            mappedIdx++;
          }
           
        }  
        ret = true;
      }
    } catch(...) {
      //TODO report why we could not connect / find CPUs
    }

    if(verbose) {
      showCpuList();
      sLog << " Done."  << std::endl << "Found " << getCpuQty() << " Cores, " << cpuIdxMap.size() << " of them run a valid DM firmware." << std::endl;
    }  
    if (cpuIdxMap.size() == 0) {throw std::runtime_error("No CPUs running a valid DM firmware found"); return false;}


    return ret;

  }

  bool CarpeDM::disconnect() {
    bool ret = false;

    if(verbose) sLog << "Disconnecting ... ";
    try { 
      ebd.close();
      ebs.close();
      cpuQty = -1;
      ret = true;
    } catch(...) {
      //TODO report why we could not disconnect
    }
    if(verbose) sLog << " Done" << std::endl;
    return ret;
  }




  void CarpeDM::completeId(vertex_t v, Graph& g) { // deduce SubID fields from ID or vice versa, depending on whether ID is defined
    

    
    std::stringstream ss;
    uint64_t id;
    uint8_t fid;
    boost::dynamic_properties dp = createParser(g); //create current property map
    /*
      //test
    auto test0 = boost::get(&myVertex::cmdDest,   g);
    auto test1 = boost::get(dnp::TMsg::SubId::sGid, dp, v);

    std::cout << "Type property: " << boost::typeindex::type_id_with_cvr<decltype(test0)>().pretty_name() << std::endl;
    std::cout << "Type property map: " << boost::typeindex::type_id_with_cvr<decltype(test1)>().pretty_name() << std::endl;
    std::cout << "Value property map: " << test1 << std::endl;
    */

    
    if (g[v].id == DotStr::Misc::sUndefined64) { // from SubID fields to ID
      //std::cout << "Input Node  " << g[v].name;
      fid = (s2u<uint8_t>(g[v].id_fid) & ID_FID_MSK); //get fid
      if (fid >= idFormats.size()) throw std::runtime_error("bad format id (FID) field in Node '" + g[v].name + "'");
      vPf& vTmp = idFormats[fid]; //choose conversion vector by fid
      id = 0;
      for(auto& it : vTmp) {  //for each format vector element 
        //use dot property tag string as key to dp map (map of tags to (maps of vertex_indices to values))
        uint64_t val = s2u<uint64_t>(boost::get(it.s, dp, v)); // use vertex index v as key in this property map to obtain value
        //std::cout << ", " << std::dec << it.s << " = " << (val & ((1 << it.bits ) - 1) ) << ", (" << (int)it.pos << ",0x" << std::hex << ((1 << it.bits ) - 1) << ")";
        id |= ((val & ((1 << it.bits ) - 1) ) << it.pos); // OR the masked and shifted value to id
      }
      
      ss.flush();
      ss << "0x" << std::hex << id;
      g[v].id = ss.str();
      //std::cout << "ID = " << g[v].id << std::endl;
    } else { //from ID to SubID fields
      id = s2u<uint8_t>(g[v].id);
      fid = ((id >> ID_FID_POS) & ID_FID_MSK);
      if (fid >= idFormats.size()) throw std::runtime_error("bad format id (FID) within ID field of Node '" + g[v].name + "'");
      vPf& vTmp = idFormats[fid];

      for(auto& it : vTmp) {
        ss.flush();
        ss << std::dec << ((id >> it.pos) &  ((1 << it.bits ) - 1) );
        boost::put(it.s, dp, v, ss.str());
      }  
    }
    
  }  


    boost::dynamic_properties CarpeDM::createParser(Graph& g) {



    boost::dynamic_properties dp(boost::ignore_other_properties);
    boost::ref_property_map<Graph *, std::string> gname( boost::get_property(g, boost::graph_name));
    dp.property(dgp::sName,     gname);
    dp.property(dep::Base::sType,               boost::get(&myEdge::type,         g));
    dp.property(dnp::Base::sName,               boost::get(&myVertex::name,       g));
    dp.property(dnp::Base::sCpu,                boost::get(&myVertex::cpu,        g));

    dp.property(dnp::Base::sType,               boost::get(&myVertex::type,       g));
    dp.property(dnp::Base::sFlags,              boost::get(&myVertex::flags,      g));
    dp.property(dnp::Base::sPatName,            boost::get(&myVertex::patName,    g));
    dp.property(dnp::Base::sPatEntry,           boost::get(&myVertex::patEntry,   g));
    dp.property(dnp::Base::sPatExit,            boost::get(&myVertex::patExit,    g));
    dp.property(dnp::Base::sBpName,             boost::get(&myVertex::bpName,     g));
    dp.property(dnp::Base::sBpEntry,            boost::get(&myVertex::bpEntry,    g));
    dp.property(dnp::Base::sBpExit,             boost::get(&myVertex::bpExit,     g));
    //Block
    dp.property(dnp::Block::sTimePeriod,        boost::get(&myVertex::tPeriod,    g));
    dp.property(dnp::Block::sGenQPrioHi,        boost::get(&myVertex::qIl,        g));
    dp.property(dnp::Block::sGenQPrioMd,        boost::get(&myVertex::qHi,        g));
    dp.property(dnp::Block::sGenQPrioLo,        boost::get(&myVertex::qLo,        g));
    //Timing Message
    dp.property(dnp::TMsg::sTimeOffs,           boost::get(&myVertex::tOffs,      g));
    dp.property(dnp::TMsg::sId,                 boost::get(&myVertex::id,         g));
      //ID sub fields
    dp.property(dnp::TMsg::SubId::sFid,         boost::get(&myVertex::id_fid,     g));
    dp.property(dnp::TMsg::SubId::sGid,         boost::get(&myVertex::id_gid,     g));
    dp.property(dnp::TMsg::SubId::sEno,         boost::get(&myVertex::id_evtno,   g));
    dp.property(dnp::TMsg::SubId::sSid,         boost::get(&myVertex::id_sid,     g));
    dp.property(dnp::TMsg::SubId::sBpid,        boost::get(&myVertex::id_bpid,    g));
    dp.property(dnp::TMsg::SubId::sBin,         boost::get(&myVertex::id_bin,     g));
    dp.property(dnp::TMsg::SubId::sReqNoB,      boost::get(&myVertex::id_reqnob,  g));
    dp.property(dnp::TMsg::SubId::sVacc,        boost::get(&myVertex::id_vacc,    g));
    dp.property(dnp::TMsg::sPar,                boost::get(&myVertex::par,        g));
    dp.property(dnp::TMsg::sTef,                boost::get(&myVertex::tef,        g));
    //Command
    dp.property(dnp::Cmd::sTimeValid,           boost::get(&myVertex::tValid,     g));
    dp.property(dnp::Cmd::sPrio,                boost::get(&myVertex::prio,       g));
    dp.property(dnp::Cmd::sQty,                 boost::get(&myVertex::qty,        g));
    dp.property(dnp::Cmd::sTimeWait,            boost::get(&myVertex::tWait,      g));
    //for .dot-cmd abuse
    dp.property(dnp::Cmd::sTarget,              boost::get(&myVertex::cmdTarget,  g));
    dp.property(dnp::Cmd::sDst,                 boost::get(&myVertex::cmdDest,    g));
    dp.property(dnp::Cmd::sDstPattern,          boost::get(&myVertex::cmdDestPat, g));                     
    dp.property(dnp::Cmd::sDstBeamproc,         boost::get(&myVertex::cmdDestBp,  g));
    dp.property(dnp::Base::sThread,             boost::get(&myVertex::thread,     g));
    

  


    return (const boost::dynamic_properties)dp;
  }   


  std::string CarpeDM::readTextFile(const std::string& fn) {
    std::string ret;
    std::ifstream in(fn);
    if(in.good()) {
      std::stringstream buffer;
      buffer << in.rdbuf();
      ret = buffer.str();
    }  
    else {throw std::runtime_error(" Could not read from file '" + fn + "'");}  

    return ret;
  }

  Graph& CarpeDM::parseDot(const std::string& s, Graph& g) {
    boost::dynamic_properties dp = createParser(g);

    try { boost::read_graphviz(s, g, dp, dnp::Base::sName); }
    catch(...) { throw; }
   
    BOOST_FOREACH( vertex_t v, vertices(g) ) { g[v].hash = hm.fnvHash(g[v].name.c_str()); } //generate hash to complete vertex information
    
    return g;
  }


  int CarpeDM::parseFwVersionString(const std::string& s) {
    
    int verMaj, verMin, verRev;
    std::vector<std::string> x;



    try { boost::split(x, s, boost::is_any_of(".")); } catch (...) {};
    if (x.size() != 3) {return FWID_BAD_VERSION_FORMAT;}

    verMaj = std::stoi (x[VERSION_MAJOR]);
    verMin = std::stoi (x[VERSION_MINOR]);
    verRev = std::stoi (x[VERSION_REVISION]);
    
    if (verMaj < 0 || verMaj > 99 || verMin < 0 || verMin > 99 || verRev < 0 || verRev > 99) {return  FWID_BAD_VERSION_FORMAT;}
    else {return verMaj * VERSION_MAJOR_MUL + verMin * VERSION_MINOR_MUL  + verRev * VERSION_REVISION_MUL;}


  }

  //returns firmware version as int <xxyyzz> (x Major Version, y Minor Version, z Revison; negative values for error codes)
  int CarpeDM::getFwVersion(uint8_t cpuIdx) {
    //FIXME replace with FW ID string constants
    const std::string tagMagic      = "UserLM32";
    const std::string tagProject    = "Project     : ";
    const std::string tagExpName    = "ftm";
    const std::string tagVersion    = "Version     : ";
    const std::string tagVersionEnd = "Platform    : ";
    std::string version;
    size_t pos, posEnd;
    struct  sdb_device& ram = myDevs.at(cpuIdx);
    vAdr fwIdAdr;

    if ((ram.sdb_component.addr_last - ram.sdb_component.addr_first + 1) < SHARED_OFFS) { return FWID_RAM_TOO_SMALL;}

    for (uint32_t adr = ram.sdb_component.addr_first + BUILDID_OFFS; adr < ram.sdb_component.addr_first + SHARED_OFFS; adr += 4) fwIdAdr.push_back(adr);
    vBuf fwIdData = ebReadCycle(ebd, fwIdAdr);
    std::string s(fwIdData.begin(),fwIdData.end());

    //check for magic word
    pos = 0;
    if(s.find(tagMagic, 0) == std::string::npos) {return FWID_BAD_MAGIC;} 
    //check for project name
    pos = s.find(tagProject, 0);
    if (pos == std::string::npos || (s.find(tagExpName, pos + tagProject.length()) != pos + tagProject.length())) {return FWID_BAD_PROJECT_NAME;} 
    //get Version string xx.yy.zz    
    pos = s.find(tagVersion, 0);
    posEnd = s.find(tagVersionEnd, pos + tagVersion.length());
    if((pos == std::string::npos) || (posEnd == std::string::npos)) {return FWID_NOT_FOUND;}
    version = s.substr(pos + tagVersion.length(), posEnd - (pos + tagVersion.length()));
    
    int ret = parseFwVersionString(version);

    return ret;
  }


  uint8_t CarpeDM::getNodeCpu(const std::string& name, bool direction) {
     
    AllocTable& at = (direction == UPLOAD ? atUp : atDown );
    uint32_t hash;
    if (!(hm.lookup(name))) {throw std::runtime_error( "Unknown Node Name '" + name + "' when lookup up hosting cpu"); return -1;} 
    hash = hm.lookup(name).get(); //just pass it on
    
    auto x = at.lookupHash(hash);
    if (!(at.isOk(x)))  {throw std::runtime_error( "Could not find Node '" + name + "' in allocation table"); return -1;}
    
    return x->cpu;
  }

  uint32_t CarpeDM::getNodeAdr(const std::string& name, bool direction, bool intExt) {
    if(name == DotStr::Node::Special::sIdle) return LM32_NULL_PTR; //idle node is resolved as a null ptr without comment

    AllocTable& at = (direction == UPLOAD ? atUp : atDown );
    uint32_t hash;
    if (!(hm.lookup(name))) {throw std::runtime_error( "Unknown Node Name '" + name + "' when lookup up address"); return LM32_NULL_PTR;} 
    hash = hm.lookup(name).get(); //just pass it on
    auto x = at.lookupHash(hash);
    if (!(at.isOk(x)))  {throw std::runtime_error( "Could not find Node in allocation table"); return LM32_NULL_PTR;}
    else            {return (intExt == INTERNAL ? at.adr2intAdr(x->cpu, x->adr) : at.adr2extAdr(x->cpu, x->adr));}
  }

 
void CarpeDM::showCpuList() {
  int expVersion = parseFwVersionString(EXP_VER);

  sLog << std::setfill(' ') << std::setw(7) << "CPU" << std::setfill(' ') << std::setw(11) << "FW found" << std::setfill(' ') << std::setw(10) << "FW exp." << std::endl;
  for (int x = 0; x < cpuQty; x++) {
    if (vFw[x] > expVersion) sLog << " ! ";
    else if (vFw[x] < expVersion) sLog << " X ";
    else sLog << "   ";
    sLog << "  " << std::dec << std::setfill(' ') << std::setw(2) << x << "   " << std::setfill('0') << std::setw(6) << vFw[x] << "   " << std::setfill('0') << std::setw(6) << expVersion;
    sLog << std::endl;
  }

}

//Returns if a hash / nodename is present on DM
  bool CarpeDM::isInHashDict(const uint32_t hash)  {

    if (atDown.isOk(atDown.lookupHash(hash))) return true;
    else return false;
  }

  bool CarpeDM::isInHashDict(const std::string& name) {
    if (!(hm.contains(name))) return false;
    return (atDown.isOk(atDown.lookupHash(hm.lookup(name).get())));
  }  

  void CarpeDM::show(const std::string& title, const std::string& logDictFile, bool direction, bool filterMeta ) {

    Graph& g        = (direction == UPLOAD ? gUp  : gDown);
    AllocTable& at  = (direction == UPLOAD ? atUp : atDown);



    std::cout << std::endl << title << std::endl;
    std::cout << std::endl << std::setfill(' ') << std::setw(4) << "Idx" << "   " << std::setfill(' ') << std::setw(4) << "S/R" << "   " << std::setfill(' ') << std::setw(4) << "Cpu" << "   " << std::setw(30) << "Name" << "   " << std::setw(10) << "Hash" << "   " << std::setw(10)  <<  "Int. Adr   "  << "   " << std::setw(10) << "Ext. Adr   " << std::endl;
    std::cout << std::endl; 



    BOOST_FOREACH( vertex_t v, vertices(g) ) {
      auto x = at.lookupVertex(v);
      
      if( !(filterMeta) || (filterMeta & !(g[v].np->isMeta())) ) {
        std::cout   << std::setfill(' ') << std::setw(4) << std::dec << v 
        << "   "    << std::setfill(' ') << std::setw(2) << std::dec << (int)(at.isOk(x) && (int)(at.isStaged(x)))  
        << " "      << std::setfill(' ') << std::setw(1) << std::dec << (int)(!(at.isOk(x)))
        << "   "    << std::setfill(' ') << std::setw(4) << std::dec << (at.isOk(x) ? (int)x->cpu : -1 )  
        << "   "    << std::setfill(' ') << std::setw(40) << std::left << g[v].name 
        << "   0x"  << std::hex << std::setfill('0') << std::setw(8) << (at.isOk(x) ? x->hash  : 0 )
        << "   0x"  << std::hex << std::setfill('0') << std::setw(8) << (at.isOk(x) ? at.adr2intAdr(x->cpu, x->adr)  : 0 ) 
        << "   0x"  << std::hex << std::setfill('0') << std::setw(8) << (at.isOk(x) ? at.adr2extAdr(x->cpu, x->adr)  : 0 )  << std::endl;
      }
    }  

    std::cout << std::endl;  
  }

   //write out dotstringfrom download graph
  std::string CarpeDM::createDot(Graph& g, bool filterMeta) {
    std::ostringstream out;


    typedef boost::property_map< Graph, node_ptr myVertex::* >::type NpMap;

    boost::filtered_graph <Graph, boost::keep_all, non_meta<NpMap> > fg(g, boost::keep_all(), make_non_meta(boost::get(&myVertex::np, g)));
    try { 
        
        if (filterMeta) {
          boost::write_graphviz(out, fg, make_vertex_writer(boost::get(&myVertex::np, fg)), 
                      make_edge_writer(boost::get(&myEdge::type, fg)), sample_graph_writer{DotStr::Graph::sDefName},
                      boost::get(&myVertex::name, fg));
        }
        else {
        
          boost::write_graphviz(out, g, make_vertex_writer(boost::get(&myVertex::np, g)), 
                      make_edge_writer(boost::get(&myEdge::type, g)), sample_graph_writer{DotStr::Graph::sDefName},
                      boost::get(&myVertex::name, g));
        }
      }
      catch(...) {throw;}

    
    return out.str();
  }

  //write out dotfile from download graph of a memunit
  void CarpeDM::writeTextFile(const std::string& fn, const std::string& s) {
    std::ofstream out(fn);
    
    if (verbose) sLog << "Writing Output File " << fn << "... ";
    if(out.good()) { out << s; }
    else {throw std::runtime_error(" Could not write to .dot file '" + fn + "'"); return;} 
    if (verbose) sLog << "Done.";
  }