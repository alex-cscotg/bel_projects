#include "dotstr.h"

namespace DotStr {

  namespace Misc {
    //pattern for uninitialised properties and their detection
    const unsigned char deadbeef[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    const std::string needle(deadbeef, deadbeef + 4);

    const std::string tHexZero     = "0x0";
    const std::string tZero        = "0";
    const std::string tUndefined64 = "0xD15EA5EDDEADBEEF";
    const std::string tUndefined32 = "0xDEADBEEF";
    const uint32_t    uUndefined32 = 0xDEADBEEF; //yeah yeah, it's not a string. I know
    const std::string tUndefined   = "undefined";

    // tag constants for both nodes and edges
    const std::string sPrioHi       = "prioil"; //FIXME string is still fitting for 'Interlock' as highest priority
    const std::string sPrioMd       = "priohi"; //FIXME string is still fitting for 'Interlock' as highest priority
    const std::string sPrioLo       = "priolo";

  }

 namespace Edge {
    // edge properties
    namespace Prop {
      namespace base {
        const std::string sType = "type";
      }
    }  

    namespace TypeVal {
      // edge type tags
      const std::string sQPrio[]      = {Misc::sPrioLo, Misc::sPrioMd, Misc::sPrioHi};
      const std::string sDstList      = "listdst";
      const std::string sDefDst       = "defdst";
      const std::string sAltDst       = "altdst";
      const std::string sBadDefDst    = "baddefdst";
      const std::string sCmdTarget    = "target";
      const std::string sCmdFlowDst   = "flowdst";
      const std::string sDynId        = "dynid";
      const std::string sDynPar0      = "dynpar0";
      const std::string sDynPar1      = "dynpar1";
      const std::string sDynTef       = "dyntef";
      const std::string sDynRes       = "dynres";
    }
  }

  namespace Node {
    
    // node properties
    namespace Prop {
      namespace base {
        const std::string sType   = "type";  
        const std::string sName   = "node_id"
        const std::string sCpu    = "cpu";   
        const std::string sFlags  = "flags"; 
      }

      namespace Block {
 
        const std::string sPeriod     = "tperiod"
        const std::string sGenQPrioHi = "qil"
        const std::string sGenQPrioMd = "qhi"
        const std::string sGenQPrioLo = "qlo"
      }  

      namespace TMsg {

        const std::string sTimeOffs = "toffs"; 
        const std::string sId       = "id";
        namespace SubId {
          const std::string sFid    = "fid";   
          const std::string sGid    = "gid";   
          const std::string sEno    = "evtno"; 
          const std::string sSid    = "sid";   
          const std::string sBpid   = "bpid";  
          const std::string sRes    = "res";
        } 
        const std::string sPar      = "par";   
        const std::string sTef      = "tef";
      }

      namespace Cmd {
     
        const std::string sValid    = "tvalid";
        const std::string sPrio     = "prio";  
        const std::string sQty      = "qty";   
        const std::string sTimeWait = "twait"; 
      }  
    }


    namespace MetaGen {
      //name prefixes, tags and suffixes for automatic meta node generation
      const std::string sDstListSuffix  = "_ListDst";
      const std::string sQPrioPrefix[]  = {"Lo", "Hi", "Il"};
      const std::string sQBufListTag    = "_QBl_";
      const std::string sQBufTag        = "_Qb_";
      const std::string s1stQBufSuffix  = "0";
      const std::string s2ndQBufSuffix  = "1";
    }


    // node type tags
    namespace TypeVal {
      const std::string sQPrio[]      = {Misc::sPrioLo, Misc::sPrioMd, Misc::sPrioHi};
      const std::string sTMsg         = "tmsg";
      const std::string sCmdNoop      = "noop";
      const std::string sCmdFlow      = "flow";
      const std::string sCmdFlush     = "flush";
      const std::string sCmdWait      = "wait";
      const std::string sBlock        = "block";
      const std::string sBlockFixed   = "blockfixed";
      const std::string sBlockAlign   = "blockalign";
      const std::string sQInfo        = "qinfo";
      const std::string sDstList      = "listdst";
      const std::string sQBuf         = "qbuf";
      const std::string sMeta         = "meta";
      const bool bMetaNode            = true;  // as comparison against isMeta() Node class member function
      const bool bRealNode            = false; //yeah yeah, it's not a string. I know

    }  
  }  

  namespace Graph {

    namespace Prop {
      const std::string sName   = "name"; 
      const std::string sRoot   = "root";
    }  
    const std::string sDefName  = "Demo";
  }
  
  //Configures how a dot will be rendered
  namespace EyeCandy {

    namespace Graph {
      const std::string sLookVert = "rankdir=TB, nodesep=0.6, mindist=1.0, ranksep=1.0, overlap=false";
      const std::string sLookHor  = "rankdir=LR, nodesep=0.6, mindist=1.0, ranksep=1.0, overlap=false";
    }
    
    namespace Node {
      namespace Block {
        const std::string sLookDef = "shape=\"rectangle\", color=\"black\"";
        const std::string sLookFix = sLookDef;
        const std::string sLookAlign = "shape=\"rectangle\", color=\"blue\"";

      }
      namespace TMsg {
        const std::string sLookDef = "shape=\"oval\", color=\"black\"";
      }  
      namespace Cmd {
        const std::string sLookDef = "shape=\"hexagon\", color=\"blue\"";

      }
      namespace Meta {
        const std::string sLookDef = "shape=\"rectangle\", color=\"gray\", tyle=\"dashed\"";
      }
    } 

    namespace Edge {
   
      const std::string sLookDefDst     = "color=\"red\"";
      const std::string sLookAltDst     = "color=\"black\"";
      const std::string sLookMeta       = "color=\"gray\"";
      const std::string sLookTarget     = "color=\"blue\"";
      const std::string sLookArgument   = "color=\"pink\"";
      const std::string sLookbad        = "color=\"orange\", style=\"dashed\"";
      
    }   
  }


}
