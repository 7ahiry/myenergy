/**
 *  \file   linear.c
 *  \brief  linearly decreasing battery
 *  \author Guillaume Chelius
 *  \date   2007
 **/
#include <include/modelutils.h>


/* ************************************************** */
/* ************************************************** */
model_t model =  {
    "Linearly decreasing battery",
    "Guillaume Chelius",
    "0.1",
    MODELTYPE_ENERGY, 
    {NULL, 0}
};

double consume_move(call_t *c, int energy);
/* ************************************************** */
/* ************************************************** */
int init(call_t *c, void *params) {
    return 0;
}

int destroy(call_t *c) {
    return 0;
}


/* ************************************************** */
/* ************************************************** */
struct nodedata {
    double energy;
    double initial;
    double tx;
    double rx;
    double idle;
    int sink;
};


/* ************************************************** */
/* ************************************************** */
int setnode(call_t *c, void *params) {
    struct nodedata *nodedata = malloc(sizeof(struct nodedata));
    param_t *param;

    /* default values */
    nodedata->energy  = 1000000;
    nodedata->initial = 1000000;
    nodedata->tx      = 1;
    nodedata->rx      = 1;
    nodedata->sink    = 0;
    //nodedata->acc     = 20;
    nodedata->idle    = nodedata->rx;

   /* get parameters */
    das_init_traverse(params);
    while ((param = (param_t *) das_traverse(params)) != NULL) {
        if (!strcmp(param->key, "energy")) {
            if (get_param_double(param->value, &(nodedata->energy))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "tx")) {
            if (get_param_double(param->value, &(nodedata->tx))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "rx")) {
            if (get_param_double(param->value, &(nodedata->rx))) {
                goto error;
            }
        }
        if (!strcmp(param->key, "sink")) {
            if (get_param_integer(param->value, &(nodedata->sink))) {
                goto error;
            }
        }
        //if (!strcmp(param->key, "acc")) {
        //    if (get_param_double(param->value, &(nodedata->acc))) {
        //        goto error;
        //    }
        //}
        if (!strcmp(param->key, "idle")) {
            if (get_param_double(param->value, &(nodedata->idle))) {
                goto error;
            }
        }
    }
    
    nodedata->initial = nodedata->energy;
    nodedata->energy = nodedata->energy - get_random_integer_range(0,10000);
    set_node_private_data(c, nodedata);
    return 0;
    
 error:
    free(nodedata);
    return -1;
}

int unsetnode(call_t *c) {
    free(get_node_private_data(c));
    return 0;
}


/* ************************************************** */
/* ************************************************** */
int bootstrap(call_t *c) {
    return 0;
}

int ioctl(call_t *c, int option, void *in, void **out) {
  struct nodedata *nodedata = get_node_private_data(c);
  //if(option[0]) {
  //  double distance = option[0] ;
  //  double speed = option[1] ;
  consume_move(c,option);   
  // fprintf(stderr,"(%d) -------------------------------------------- %i %f\n", c->node, option,energy );
  //} else {
    //fprintf(stderr,"Wrong parameter for IOCTL function in myenergy library!\n");
  //}
  //
  // fprintf(stderr,"%lli (%d) -------------------------------------------- %i\n", get_time(), c->node,option );
  int temp = (int)(nodedata->energy*100/nodedata->initial);
  return temp;
}


/* ************************************************** */
/* ************************************************** */
void consume_tx(call_t *c, uint64_t duration, double txdBm) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return;
  
    nodedata->energy -= duration * nodedata->tx; 
    if (nodedata->energy <= 0) {
        nodedata->energy = 0;
        node_kill(c->node);
    }
    return;
}

double consume_move(call_t *c, int energy) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return 0;
    
    // This include harvesting... 
    nodedata->energy = nodedata->energy + energy ; 
    //fprintf(stderr, "NRJ (%d) %lf %lf %lf\n", c->node, speed, distance,nodedata->energy );
    if (nodedata->energy <= 0) {
        nodedata->energy = 0;
        node_kill(c->node);
    }

    if (nodedata->energy > nodedata->initial){
      nodedata->energy = nodedata->initial;
    }
    return nodedata->energy;
}

void consume_rx(call_t *c, uint64_t duration) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return;

    nodedata->energy -= duration * nodedata->rx; 
    if (nodedata->energy <= 0) {
        nodedata->energy = 0;
        node_kill(c->node);
    }
    return;
}

void consume_idle(call_t *c, uint64_t duration) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return;

    nodedata->energy -= duration * nodedata->idle; 
    if (nodedata->energy <= 0) {
        nodedata->energy = 0;
        node_kill(c->node);
    }
    return;
}

void consume(call_t *c, double energy) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return;

    nodedata->energy -= energy; 
    if (nodedata->energy <= 0) {
        nodedata->energy = 0;
        node_kill(c->node);
    }
    return;
}

double energy_consumed(call_t *c) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return 0;

    return nodedata->initial - nodedata->energy;
}

double energy_remaining(call_t *c) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return 0;

    return nodedata->energy;
}

double energy_status(call_t *c) {
    struct nodedata *nodedata = get_node_private_data(c);
    if (c->node == nodedata->sink)
      return 0;

    double status = nodedata->energy / nodedata->initial;
    if (nodedata->energy <= 0) {
        return 0;
    } else if ((status >= 0) && (status <= 1)) {
        return status;
    } else {
        return 0;
    }
}


/* ************************************************** */
/* ************************************************** */
energy_methods_t methods = {consume_tx, 
                            consume_rx,
                            consume_idle,
                            consume_move,
                            consume,
                            energy_consumed,
                            energy_remaining,
                            energy_status};
