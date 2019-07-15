#ifndef __LNET_H
#define __LNET_H 1

#include "net.h"
#include "eth.h"

typedef struct lnet lnet_t;
typedef struct lendpoint lendpoint_t;

lnet_t *lnet_switch(int count);
ifnet_t *lnet_endpoint(lnet_t *lan, inet_t *net);

#endif  /* __LNET_H */
