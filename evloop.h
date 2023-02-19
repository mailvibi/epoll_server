#ifndef __EVELOOP_H__
#define __EVELOOP_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct ev_handle* ev_handle_t;
typedef enum ev_res {EV_OK = 1, EV_ERR} ev_res_t;
typedef enum ev_trig {EV_TRIG_READ = 0, EV_TRIG_WRITE, EV_TRIG_INVAL} ev_trig_t;
typedef bool (*ev_cb_t) (int fd, ev_trig_t trig, void *cxt);

ev_res_t ev_init(ev_handle_t *evh, uint32_t max_events);
ev_res_t ev_add(ev_handle_t evh, int fd, ev_trig_t trig, ev_cb_t cb, void *cxt);
ev_res_t ev_del(ev_handle_t evh, int fd, ev_trig_t trig);
ev_res_t ev_run(ev_handle_t evh);
ev_res_t ev_deinit(ev_handle_t evh);


#endif /*__EVELOOP_H__*/