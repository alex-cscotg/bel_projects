#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/container/vector.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

class Node;

typedef boost::shared_ptr<Node> node_ptr;
typedef boost::container::vector<uint8_t> vBuf;
typedef boost::container::vector<node_ptr> npBuf;
typedef boost::container::vector<uint8_t>::iterator itBuf;


  typedef struct {
    std::string name;
    node_ptr np;
  } myVertex;




  typedef struct {
    int type;
    boost::function<uint64_t(void)> getTimeParent;
    boost::function<uint64_t(void)> getTimeChild;
  } myEdge;


  typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::bidirectionalS, myVertex, myEdge > Graph;
  typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;


inline void uint16ToBytes(itBuf ib, uint16_t val) {
  ib[1]  = (uint8_t)val;
  ib[0]  = (uint8_t)(val >> 8);
}

inline uint16_t bytesToUint16(itBuf ib) {
  return (uint16_t)ib[0] | ((uint16_t)ib[1] << 8);
}

inline void uint32ToBytes(itBuf ib, uint32_t val) {
  uint16ToBytes(ib +0, (uint16_t)(val >> 16));
  uint16ToBytes(ib +2, (uint16_t)val);
}

inline uint32_t bytesToUint32(itBuf ib) {
  return ((uint32_t)bytesToUint16(ib+0) << 16) | (uint32_t)bytesToUint16(ib +2);
}

inline void uint64ToBytes(itBuf ib, uint64_t val) {
  uint32ToBytes(ib +0, (uint32_t)(val >> 32));
  uint32ToBytes(ib +4, (uint32_t)val);
}

inline uint64_t bytesToUint64(itBuf ib) {
  return ((uint64_t)bytesToUint32(ib +0) << 32) | (uint64_t)bytesToUint32(ib +4);
}


#endif
