#include <boost/shared_ptr.hpp>
#include <algorithm>  
#include <stdio.h>
#include <iostream>
#include <string>
#include <inttypes.h>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string.hpp>

#include "common.h"
#include "propwrite.h"
#include "graph.h"
#include "carpeDM.h"
#include "visitoruploadcrawler.h"
#include "visitordownloadcrawler.h"
#include "dotstr.h"
#include "idformat.h"

namespace dnp = DotStr::Node::Prop;
namespace dnt = DotStr::Node::TypeVal;
namespace dep = DotStr::Edge::Prop;
namespace det = DotStr::Edge::TypeVal;;
namespace dnm = DotStr::Node::MetaGen;
  

  //TODO NC Traffic Verification

  //TODO CPU Load Balancer

     
  vAdr CarpeDM::getUploadAdrs(){
    vAdr ret;
    uint32_t adr;

    //add all Bmp addresses to return vector
    for(unsigned int i = 0; i < atUp.getMemories().size(); i++) {
      //generate addresses of Bmp's address range
      for (adr = atUp.adr2extAdr(i, atUp.getMemories()[i].sharedOffs); adr < atUp.adr2extAdr(i, atUp.getMemories()[i].startOffs); adr += _32b_SIZE_) ret.push_back(adr);
    }

    //add all Node addresses to return vector
    for (auto& it : atUp.getTable().get<CpuAdr>()) {
      //generate address range for all nodes staged for upload
      if(it.staged) {
       //std::cout << std::hex << "Adding Node @ 0x" << atUp.adr2extAdr(it.cpu, it.adr) << std::endl;

       for (adr = atUp.adr2extAdr(it.cpu, it.adr); adr < atUp.adr2extAdr(it.cpu, it.adr + _MEM_BLOCK_SIZE); adr += _32b_SIZE_ ) ret.push_back(adr); 
     }
    }    
    return ret;
  }

  vBuf CarpeDM::getUploadData()  {
    vBuf ret;
    ret.clear();
    
    size_t bmpSum = 0;
    for(unsigned int i = 0; i < atUp.getMemories().size(); i++) { bmpSum += atUp.getMemories()[i].bmpSize; }
    //FIXME Careful, if bmpSum is not aligned this can kill!
    // std::cout << "mem reserved " << bmpSum + atUp.getSize() * _MEM_BLOCK_SIZE << " equals " << (bmpSum + atUp.getSize() * _MEM_BLOCK_SIZE) / _MEM_BLOCK_SIZE << " nodes, " << atUp.getMemories().size() << " memory" << std::endl; 
    ret.reserve( bmpSum + atUp.getSize() * _MEM_BLOCK_SIZE); // preallocate memory for BMPs and all Nodes
    
    for(unsigned int i = 0; i < atUp.getMemories().size(); i++) {
      auto& tmpBmp = atUp.getMemories()[i].getBmp();
      //vHexDump("bmpUpb4", tmpBmp);
      //atUp.getMemories()[i].syncBmpToPool(); //sync Bmp to Pool
      //vHexDump("bmpUpafter", tmpBmp);
      
      //std::cout << "test bmp size " << tmpBmp.size() << " iterator diff " << tmpBmp.end() - tmpBmp.begin()  << " aligned size " << atUp.getMemories()[i].bmpSize << " bits " << atUp.getMemories()[i].bmpBits << std::endl;
      
      ret.insert( ret.end(), tmpBmp.begin(), tmpBmp.end() ); //add Bmp to to return vector  
    }
    //std::cout << "passed bmp" << std::endl;
    //add all node buffers to return vector
    //atUp.debug();

    for (auto& it : atUp.getTable().get<CpuAdr>()) {
      if(it.staged) {
        //hexDump(gUp[it.v].name.c_str(), (const uint8_t*)it.b, _MEM_BLOCK_SIZE); 
        ret.insert( ret.end(), it.b, it.b + _MEM_BLOCK_SIZE );
      } // add all nodes staged for upload
    }
    //std::cout << "passed nodes" << std::endl;
    return ret;
  }



  void CarpeDM::generateDstLst(Graph& g, vertex_t v) {
    const std::string name = g[v].name + dnm::sDstListSuffix;
    hm.add(name);
    vertex_t vD = boost::add_vertex(myVertex(name, g[v].cpu, hm.lookup(name).get(), nullptr, dnt::sDstList, DotStr::Misc::sHexZero), g);
    boost::add_edge(v,   vD, myEdge(det::sDstList), g);
  }  

  void CarpeDM::generateQmeta(Graph& g, vertex_t v, int prio) {
    const std::string nameBl = g[v].name + dnm::sQBufListTag + dnm::sQPrioPrefix[prio];
    const std::string nameB0 = g[v].name + dnm::sQBufTag     + dnm::sQPrioPrefix[prio] + dnm::s1stQBufSuffix;
    const std::string nameB1 = g[v].name + dnm::sQBufTag     + dnm::sQPrioPrefix[prio] + dnm::s2ndQBufSuffix;
    hm.add(nameBl);
    hm.add(nameB0);
    hm.add(nameB1);
    vertex_t vBl = boost::add_vertex(myVertex(nameBl, g[v].cpu, hm.lookup(nameBl).get(), nullptr, dnt::sQInfo, DotStr::Misc::sHexZero), g);
    vertex_t vB0 = boost::add_vertex(myVertex(nameB0, g[v].cpu, hm.lookup(nameB0).get(), nullptr, dnt::sQBuf,  DotStr::Misc::sHexZero), g);
    vertex_t vB1 = boost::add_vertex(myVertex(nameB1, g[v].cpu, hm.lookup(nameB1).get(), nullptr, dnt::sQBuf,  DotStr::Misc::sHexZero), g);
    boost::add_edge(v,   vBl, myEdge(det::sQPrio[prio]), g);
    boost::add_edge(vBl, vB0, myEdge(det::sMeta),    g);
    boost::add_edge(vBl, vB1, myEdge(det::sMeta),    g);
    
  }

  void CarpeDM::generateBlockMeta(Graph& g) {
   Graph::out_edge_iterator out_begin, out_end, out_cur;
    
    BOOST_FOREACH( vertex_t v, vertices(g) ) {
      std::string cmp = g[v].type;
      
      if ((cmp == dnt::sBlockFixed) || (cmp == dnt::sBlockAlign) || (cmp == dnt::sBlock) ) {
        boost::tie(out_begin, out_end) = out_edges(v,g);
        //check if it already has queue links / Destination List
        bool hasIl=false, hasHi=false, hasLo=false, hasMultiDst=false, hasDstLst=false;
        for (out_cur = out_begin; out_cur != out_end; ++out_cur)
        { 
          
          if (g[*out_cur].type == det::sQPrio[PRIO_IL]) hasIl       = true;
          if (g[*out_cur].type == det::sQPrio[PRIO_HI]) hasHi       = true;
          if (g[*out_cur].type == det::sQPrio[PRIO_LO]) hasLo       = true;
          if (g[*out_cur].type == det::sAltDst)         hasMultiDst = true;
          if (g[*out_cur].type == det::sDstList)        hasDstLst   = true;
        }
        //create requested Queues / Destination List
        if (g[v].qIl != "0" && !hasIl ) { generateQmeta(g, v, PRIO_IL); }
        if (g[v].qHi != "0" && !hasHi ) { generateQmeta(g, v, PRIO_HI); }
        if (g[v].qLo != "0" && !hasLo ) { generateQmeta(g, v, PRIO_LO); }
        if((hasMultiDst | ((g[v].qIl != "0") | hasIl) | ((g[v].qHi != "0") | hasHi) |  ((g[v].qLo != "0") | hasLo)) & !hasDstLst)    { generateDstLst(g, v);         }

      }
    }  
  }

  void CarpeDM::updateListDstStaging(vertex_t v) {
    Graph::out_edge_iterator out_begin, out_end, out_cur;
    Graph& g = gUp;
    AllocTable& at = atUp;
    //std::cout << g[v].name << "'s alt destination list changed" << std::endl;
    // the changed edge leads to Alternative Dst, find and stage the block's dstList 
    boost::tie(out_begin, out_end) = out_edges(v,g);  
    for (out_cur = out_begin; out_cur != out_end; ++out_cur) {
      if (g[*out_cur].type == det::sDstList) {
        auto dst = at.lookupVertex(target(*out_cur, g));
        if (at.isOk(dst)) { at.setStaged(dst); 
        //std::cout << "staged " << g[dst->v].name  << std::endl; 
        }// if we found a Dst List, stage it
        else throw std::runtime_error("Dst List '" + g[dst->v].name + "' was not allocated, this is very bad");
        break;
      }
    }
  }

  void CarpeDM::updateStaging(vertex_t v, edge_t e)  {
    // staging changes
    Graph& g = gUp;
    AllocTable& at = atUp;
    

    if (g[e].type == det::sDefDst || g[e].type == det::sAltDst ) {
      updateListDstStaging(v); // stage source block's Destination List
    }
    
    if (g[e].type != det::sAltDst ) {
      auto x = at.lookupVertex(v);
      if(at.isOk(x)) {at.setStaged(x); 
        //std::cout << "staged " << g[v].name  << std::endl;
      }
      else throw std::runtime_error("Node '" + g[v].name + "' was not allocated, this is very bad");
    }

  }


  void CarpeDM::mergeUploadDuplicates(vertex_t borg, vertex_t victim) {
    Graph& g = gUp;

    // add all of nod 'victim's edges to node 'borg'. Resistance is futile.
    //sLog << g[borg].name << " @ " << borg << " consumes " << g[victim].name << " @ " << victim << std::endl;

    Graph::out_edge_iterator out_begin, out_end, out_cur;
    boost::tie(out_begin, out_end) = out_edges(victim, g);
    
    // out edges
    for (out_cur = out_begin; out_cur != out_end; ++out_cur) {
      //sLog << " moving out edge " << std::endl;
      boost::add_edge(borg, target(*out_cur,g), (myEdge){boost::get(&myEdge::type, g, *out_cur)}, g);
      boost::remove_edge(*out_cur, g);
      updateStaging(borg, *out_cur); 
    }  
    
    // in egdes
    Graph::in_edge_iterator in_begin, in_end, in_cur;
    boost::tie(in_begin, in_end) = in_edges(victim, g);
    for (in_cur = in_begin; in_cur != in_end; ++in_cur) {
      //sLog << " moving in edge " << std::endl;
      boost::add_edge(source(*in_cur,g), borg, (myEdge){boost::get(&myEdge::type, g, *in_cur)}, g);
      boost::remove_edge(*in_cur, g);
       
    }
  }

  void CarpeDM::prepareUpload() {
    
    std::string cmp;
    uint32_t hash;
    uint8_t cpu;
    int allocState;
    

    //allocate and init all new vertices
    BOOST_FOREACH( vertex_t v, vertices(gUp) ) {
      //std::string name = boost::get_property(gUp, boost::graph_name) + "." + gUp[v].name;
      std::string name = gUp[v].name;
      //if (!(hm.lookup(name)))                   {throw std::runtime_error("Node '" + name + "' was unknown to the hashmap"); return;}
      hash = gUp[v].hash;
      cpu  = s2u<uint8_t>(gUp[v].cpu);
      //FIXME Careful! CPU indices in the (intermediary) .dot do not necessarily match the vector indices. Use the fucking cpuIdx map to translate!


      amI it = atUp.lookupHash(hash); //if we already have a download entry, keep allocation, but update vertex index
      if (!(atUp.isOk(it))) {
        //  
        //sLog << "Adding " << name << std::endl;
        allocState = atUp.allocate(cpu, hash, v, true);
        if (allocState == ALLOC_NO_SPACE)         {throw std::runtime_error("Not enough space in CPU " + std::to_string(cpu) + " memory pool"); return; }
        if (allocState == ALLOC_ENTRY_EXISTS)     {throw std::runtime_error("Node '" + name + "' would be duplicate in graph."); return; }
        // getting here means alloc went okay
        it = atUp.lookupHash(hash);
      }  
      
      //TODO Find something better than stupic cast to ptr
      //Ugly as hell. But otherwise the bloody iterator will only allow access to MY alloc buffers (not their pointers!) as const!
      auto* x = (AllocMeta*)&(*it);

      // add timing node data objects to vertices
      if(gUp[v].np == nullptr) {

        cmp = gUp[v].type;


             if (cmp == dnt::sTMsg)        {completeId(v, gUp); // create ID from SubId fields or vice versa
                                            gUp[v].np = (node_ptr) new  TimingMsg(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tOffs), s2u<uint64_t>(gUp[v].id), s2u<uint64_t>(gUp[v].par), s2u<uint32_t>(gUp[v].tef), s2u<uint32_t>(gUp[v].res)); }
        else if (cmp == dnt::sCmdNoop)     {gUp[v].np = (node_ptr) new       Noop(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tOffs), s2u<uint64_t>(gUp[v].tValid), s2u<uint8_t>(gUp[v].prio), s2u<uint8_t>(gUp[v].qty)); }
        else if (cmp == dnt::sCmdFlow)     {gUp[v].np = (node_ptr) new       Flow(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tOffs), s2u<uint64_t>(gUp[v].tValid), s2u<uint8_t>(gUp[v].prio), s2u<uint8_t>(gUp[v].qty)); }
        else if (cmp == dnt::sCmdFlush)    {gUp[v].np = (node_ptr) new      Flush(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tOffs), s2u<uint64_t>(gUp[v].tValid), s2u<uint8_t>(gUp[v].prio),
                                                                              s2u<bool>(gUp[v].qIl), s2u<bool>(gUp[v].qHi), s2u<bool>(gUp[v].qLo), s2u<uint8_t>(gUp[v].frmIl), s2u<uint8_t>(gUp[v].toIl), s2u<uint8_t>(gUp[v].frmHi),
                                                                              s2u<uint8_t>(gUp[v].toHi), s2u<uint8_t>(gUp[v].frmLo), s2u<uint8_t>(gUp[v].toLo) ); }
        else if (cmp == dnt::sCmdWait)     {gUp[v].np = (node_ptr) new       Wait(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tOffs), s2u<uint64_t>(gUp[v].tValid), s2u<uint8_t>(gUp[v].prio), s2u<uint64_t>(gUp[v].tWait)); }
        else if (cmp == dnt::sBlock)       {gUp[v].np = (node_ptr) new BlockFixed(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tPeriod) ); }
        else if (cmp == dnt::sBlockFixed)  {gUp[v].np = (node_ptr) new BlockFixed(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tPeriod) ); }
        else if (cmp == dnt::sBlockAlign)  {gUp[v].np = (node_ptr) new BlockAlign(gUp[v].name, x->hash, x->cpu, x->b, 0, s2u<uint64_t>(gUp[v].tPeriod) ); }
        else if (cmp == dnt::sQInfo)       {gUp[v].np = (node_ptr) new   CmdQMeta(gUp[v].name, x->hash, x->cpu, x->b, 0);}
        else if (cmp == dnt::sDstList)     {gUp[v].np = (node_ptr) new   DestList(gUp[v].name, x->hash, x->cpu, x->b, 0);}
        else if (cmp == dnt::sQBuf)        {gUp[v].np = (node_ptr) new CmdQBuffer(gUp[v].name, x->hash, x->cpu, x->b, 0);}
        else if (cmp == dnt::sMeta)        {throw std::runtime_error("Pure meta type not yet implemented"); return;}
        //FIXME try to get info from download
        else                        {throw std::runtime_error("Node <" + gUp[v].name + ">'s type <" + cmp + "> is not supported!\nMost likely you forgot to set the type attribute or accidentally created the node by a typo in an edge definition."); return;}
      }  
    }
   
    // Crawl vertices and serialise their data objects for upload
    BOOST_FOREACH( vertex_t v, vertices(gUp) ) {
      
      gUp[v].np->accept(VisitorUploadCrawler(gUp, v, atUp)); 
      
      //Check if all mandatory fields were properly initialised
      std::string haystack(gUp[v].np->getB(), gUp[v].np->getB() + _MEM_BLOCK_SIZE);
      std::size_t n = haystack.find(DotStr::Misc::needle);

      bool foundUninitialised = (n != std::string::npos);

      if(verbose || foundUninitialised) {
        sLog << std::endl;
        hexDump(gUp[v].name.c_str(), (void*)haystack.c_str(), _MEM_BLOCK_SIZE);
      }

      if(foundUninitialised) {
        throw std::runtime_error("Node '" + gUp[v].name + "'contains uninitialised elements!\nMisspelled/forgot a mandatory property in .dot file ?"); 
      } 
    }

  }

  int CarpeDM::upload() {
    vBuf vUlD = getUploadData();
    vAdr vUlA = getUploadAdrs();

    //Upload
    ebWriteCycle(ebd, vUlA, vUlD);
    if(verbose) sLog << "Done." << std::endl;
    return vUlD.size();
  }

  void CarpeDM::baseUploadOnDownload() {
    //init up graph from down
    gUp.clear(); //Necessary?
    atUp.clear();
    atUp.clearMemories();
    download();
    atUp = atDown;
    // for some reason, copy_graph does not copy the name
    //boost::set_property(gTmp, boost::graph_name, boost::get_property(g, boost::graph_name));
    copy_graph(gDown, gUp);
    //now, we need to change the buffer pointers in the copied nodes, as they still point to buffers in upload allocation table

    BOOST_FOREACH( vertex_t v, vertices(gUp) ) {
      auto* x = (AllocMeta*)&(*atUp.lookupVertex(v));
      gUp[v].np->setB(x->b);
    } 
  }

  void CarpeDM::addition(Graph& gTmp) {

    vertex_map_t vertexMap, duplicates;
    
    generateBlockMeta(gTmp); //auto generate desired Block Meta Nodes

    //find and list all duplicates i.e. docking points between trees
 
    //probably a more elegant solution out there, but I don't have the time for trial and error on boost property maps.
    BOOST_FOREACH( vertex_t v, vertices(gUp) ) { 
      BOOST_FOREACH( vertex_t w, vertices(gTmp) ) { 
        if (gTmp[w].hash == gUp[v].hash) { duplicates[v] = w;
          //sLog << gTmp[w].name << " gTmp " << w << " <-> " << gUp[v].name << " gUp " << v << std::endl; 
        } 
      }  
    }

    //merge graphs (will lead to disjunct trees with duplicates at overlaps), but keep the mapping for vertex merge
    boost::associative_property_map<vertex_map_t> vertexMapWrapper(vertexMap);
    copy_graph(gTmp, gUp, boost::orig_to_copy(vertexMapWrapper));
    //for(auto& it : vertexMap ) {sLog <<  "gTmp " << gTmp[it.first].name << " @ " << it.first << " gUp " << it.second << std::endl; }
    //merge duplicate nodes
    for(auto& it : duplicates ) { 
      //sLog <<  it.first << " <- " << it.second << "(" << vertexMap[it.second] << ")" << std::endl; 
      mergeUploadDuplicates(it.first, vertexMap[it.second]); 
    }

    //now remove duplicates
    for(auto& itDup : duplicates ) { 
      boost::clear_vertex(vertexMap[itDup.second], gUp); 
      boost::remove_vertex(vertexMap[itDup.second], gUp);
      //remove_vertex() changes the vertex vector, as it must stay contignuous. descriptors higher than he removed one therefore need to be decremented by 1
      for( auto& updateMapIt : vertexMap) {if (updateMapIt.first > itDup.second) updateMapIt.second--; }
    }

    writeUpDotFile("inspect.dot", false);

    prepareUpload();
    atUp.updateBmps();
  
  }

  void CarpeDM::pushMetaNeighbours(vertex_t v, Graph& g, vertex_set_t& s) {
    
    //recursively find all adjacent meta type vertices
    BOOST_FOREACH( vertex_t w, adjacent_vertices(v, g)) {
      if (g[w].np == nullptr) {throw std::runtime_error("Node " + g[w].name + " does not have a data object, this is bad");}
      if (g[w].np->isMeta()) {
        s.insert(w);
        //sLog <<  "Added Meta Child " << g[w].name << " to del map " << std::endl; 
        pushMetaNeighbours(w, g, s);
      } else {
        //sLog <<  g[w].name << " is not meta, stopping crawl here" << std::endl; 
      }
    }
  }

  void CarpeDM::subtraction(Graph& gTmp) {

    vertex_map_t vertexMap;
    vertex_set_t toDelete;
    
    //probably a more elegant solution out there, but I don't have the time for trial and error on boost property maps.
    //create 1:1 vertex map for all vertices in gUp initially marked for deletion. Also add all their meta children to leave no loose ends 
    
    BOOST_FOREACH( vertex_t v, vertices(gUp) ) vertexMap[v] = v;  

    bool found; 
    BOOST_FOREACH( vertex_t w, vertices(gTmp) ) {
      //sLog <<  "Looking at " << gTmp[w].name << std::endl;
      found = false; 
      BOOST_FOREACH( vertex_t v, vertices(gUp) ) {
        if ((gTmp[w].hash == gUp[v].hash)) {
          found = true;
          if (gTmp[w].type != DotStr::Misc::sUndefined) {
            
            toDelete.insert(v);                   // add the node
            //sLog <<  "Added Node " << gTmp[w].name << " of type " << gTmp[w].type << " to del map " << std::endl;   
            pushMetaNeighbours(v, gUp, toDelete); // add all of its meta children as well
          } else {}
          break;
        }
      }
      if (!found) { sLog <<  "Skipping unknown Node " << gTmp[w].name << std::endl;   } 
    }

    /*
    sLog <<  "Map b4 " << std::endl;
    for (auto it : vertexMap) sLog <<  it.first << " -> " << it.second << std::endl;
    */

    //check staging, vertices might have lost children
    for(auto& vd : toDelete ) {
      //check out all parents (sources) of this to be deleted node, update their staging
      Graph::in_edge_iterator in_begin, in_end, in_cur;
      
      boost::tie(in_begin, in_end) = in_edges(vertexMap[vd], gUp);  
      for (in_cur = in_begin; in_cur != in_end; ++in_cur) { 
        updateStaging(source(*in_cur, gUp), *in_cur);
      }
    }
    
    //remove designated vertices
    for(auto& vd : toDelete ) {  
      //sLog <<  "Removing Node " << gUp[vertexMap[vd]].name << std::endl;  
      atUp.deallocate(gUp[vertexMap[vd]].hash); //using the hash is independent of vertex descriptors, so no remapping necessary yet
      boost::clear_vertex(vertexMap[vd], gUp); 
      boost::remove_vertex(vertexMap[vd], gUp); 
      
      //FIXME this removal scheme is crap ! Graph vertices and edges ought to be kept in setS or listS so iterators stay valid on removal of other vertices
      //However, the cure is worse than the disease, graphviz_write, copy_graph all fuck around if we do, as they require vertex descriptors wich setS and listS don't provide ... leave for now

      //remove_vertex() changes the vertex vector, as it must stay contignuous. descriptors higher than he removed one therefore need to be decremented by 1
      for( auto& updateMapIt : vertexMap) {if (updateMapIt.first > vd) updateMapIt.second--; }
    }

    /*
    sLog <<  "Map after " << std::endl;
    for (auto it : vertexMap) sLog <<  it.first << " -> " << it.second << std::endl;

    atUp.debug();
    */

    writeUpDotFile("inspect.dot", false);

    //now we have a problem: all vertex descriptors in the alloctable just got invalidated by the removal ... repair them
    
    std::vector<amI> itAtVec; //because elements change order during loop, we need to store iterators first
    for( amI it = atUp.getTable().begin(); it != atUp.getTable().end(); it++) { itAtVec.push_back(it); }



    for( auto itIt : itAtVec ) {  //now we can safely iterate over the alloctable iterators
      //sLog << "Changing " << std::hex << "0x" << it->hash << " index from " << it->v; 
      atUp.modV(itIt, vertexMap[itIt->v]);
      //sLog << " to " << it->v << std::endl; ; 
    }
    //atUp.debug();
    

    prepareUpload();
    atUp.updateBmps();  
    
  }

  void CarpeDM::nullify() {
    gUp.clear(); //Necessary?
    atUp.clear();
    atUp.clearMemories();
    gDown.clear(); //Necessary?
    atDown.clear();
    atDown.clearMemories();

  }

  //high level functions for external interface

  int CarpeDM::add(Graph& g) {
    baseUploadOnDownload();
    addition(g);
    writeUpDotFile("upload.dot", false);

    return upload();
  } 

  int CarpeDM::remove(Graph& g) {
    baseUploadOnDownload();
    subtraction(g);
    //writeUpDotFile("upload.dot", false);
    return upload();
  }


  int CarpeDM::keep(Graph& g) {
    Graph gTmpRemove;
    Graph& gTmpKeep = g;

    generateBlockMeta(gTmpKeep);

    writeDotFile("inspect.dot", gTmpKeep, false);
    baseUploadOnDownload();
    
    bool found; 
    BOOST_FOREACH( vertex_t w, vertices(gUp) ) {
      //sLog <<  "Scanning " << gUp[w].name << std::endl;
      found = false; 
      BOOST_FOREACH( vertex_t v, vertices(gTmpKeep) ) {
        if ((gTmpKeep[v].name == gUp[w].name)) {
          found = true;
          //sLog <<  "Keeping Node " << gUp[w].name << std::endl;
          break;

        }
      }
      if (!found) { boost::add_vertex(myVertex(gUp[w]), gTmpRemove);
        //sLog <<  "Deleting Node " << gUp[w].name << std::endl;  
      }
    }
    
    subtraction(gTmpRemove);
    writeUpDotFile("upload.dot", false);

    return upload();
  }   

  int CarpeDM::clear() {
    nullify();
    return upload();
  }

  int CarpeDM::overwrite(Graph& g) {
    nullify();
    addition(g);
    writeUpDotFile("upload.dot", false);

    return upload();

  }