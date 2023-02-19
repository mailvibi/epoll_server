#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>

#include "evloop.h"
#include "dbg.h"

#define MAX_EVENTS (1024)

static const char *ev_name[EV_TRIG_INVAL] = {
                                            [EV_TRIG_READ] = "EV_TRIG_READ",
                                            [EV_TRIG_WRITE] = "EV_TRIG_WRITE", 
                                        };

typedef struct event {
    bool in_use;
    int fd;
    ev_cb_t cb[EV_TRIG_INVAL];
    void *cxt[EV_TRIG_INVAL];
    bool trig[EV_TRIG_INVAL];
} ev_t;

struct ev_handle {
    int max_events;
    ev_t *sub_events;
    int epollfd;
    volatile bool stop;
};

ev_res_t ev_init(struct ev_handle **evh, uint32_t max_events)
{
    struct ev_handle *eh;
    if (evh == NULL || max_events > MAX_EVENTS || max_events == 0) {
        err("Invalid options to initialize event given");
        return EV_ERR;
    } 
    eh = calloc(sizeof(struct ev_handle), 1);
    if (eh == NULL) {
        err("Failed allocating event handle");
        return EV_ERR;
    }
    eh->sub_events = calloc(sizeof(ev_t), max_events);
    if (eh->sub_events == NULL) {
        err("Memory Allocation failure : cannot allocate sub_events");
        goto err;
    }
    eh->stop = false;
    for (int i = 0 ; i < max_events; i++) {
        eh->sub_events[i].in_use = false;
    }
    eh->epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (eh->epollfd < 0) {
        perr("epoll_create1 failed");
        goto err;
    }

    *evh = eh;
    return EV_OK;
err:
    if (eh) {
        if (eh->sub_events)
            free(eh->sub_events);
        free(eh);
    }
    return EV_ERR;
}

ev_res_t ev_add(ev_handle_t evh, int fd, ev_trig_t trig, ev_cb_t cb, void *cxt)
{
    if (evh == NULL) {
        err("Invalid event handle");
        return EV_ERR;
    }
    if (fd > (evh->max_events - 1)) {
        err("Invalid fd %d", fd);
        return EV_ERR;
    }
    if (trig != EV_TRIG_READ && trig != EV_TRIG_WRITE) {
        err("Invalid trigger");
        return EV_ERR;
    }

    ev_t *ev = &evh->sub_events[fd];

    if (ev->fd && ev->fd != fd) {
        err("Non Matching event fd ev->fd(%d) != fd(%d)", ev->fd, fd);
        return EV_ERR;
    }
    bool is_overwritten = (ev->in_use && ev->trig[trig]);
    if (is_overwritten) {
        err("Overwriting of trigger event not allowed");
        return EV_ERR;
    }

    struct epoll_event ee = {0};
    ev->trig[trig] = true;
    ev->cxt[trig] = cxt;
    ev->cb[trig] = cb;

    ee.events = (trig == EV_TRIG_READ) ? EPOLLIN : EPOLLOUT;
    ee.data.fd = fd;

    int op = ev->in_use ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    int ret = epoll_ctl(evh->epollfd, op, fd, &ee);
    if (ret) {
        perr("epoll_ctl");
        ev->cxt[trig] = NULL;
        ev->cb[trig] = NULL;
        ev->trig[trig] = true;
        return EV_ERR;
    }
    ev->in_use = true;
    return EV_OK;
}

ev_res_t ev_del(ev_handle_t evh, int fd, ev_trig_t trig)
{
    if (evh == NULL) {
        err("Invalid event handle");
        return EV_ERR;
    }
    if (fd > (evh->max_events - 1)) {
        err("Invalid fd %d", fd);
        return EV_ERR;
    }
    if (trig != EV_TRIG_READ && trig != EV_TRIG_WRITE) {
        err("Invalid trigger");
        return EV_ERR;
    }
    if (!evh->sub_events[fd].in_use || !evh->sub_events[fd].trig[trig]) {
        err("event is not added");
        return EV_ERR;
    }
    bool both_event_set = evh->sub_events[fd].trig[EV_TRIG_READ] && evh->sub_events[fd].trig[EV_TRIG_WRITE];
    int op = both_event_set ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    struct epoll_event ee = {0};
    ee.events = (evh->sub_events[fd].trig[trig] == EV_TRIG_READ) ? EPOLLIN : EPOLLOUT;
    ee.data.fd = fd;
        
    int ret = epoll_ctl(evh->epollfd, op, fd, &ee);
    if (ret) {
        perr("epoll_ctl");
        return EV_ERR;
    }
    evh->sub_events[fd].trig[trig] = false;
    evh->sub_events[fd].cb[trig] = NULL;
    evh->sub_events[fd].cxt[trig] = NULL;
    
    if (!both_event_set)
        evh->sub_events[fd].in_use = false;
    return EV_OK;
}

static bool process_ev_trig(ev_t *e, ev_trig_t trig)
{
    if (!e->trig[trig]) {
        err("pollin received for fd %d but %s not set", e->fd, ev_name[trig]);
        return false;
    }
    if (e->cb[trig] == NULL) {
        err("callback for trigger %s not set for fd %d", ev_name[trig], e->fd);
        return false;
    }
    dbg("triggering callback for %s, fd = %d", ev_name[trig], e->fd);
    return e->cb[EV_TRIG_READ](e->fd, trig, e->cxt[trig]);
}

ev_res_t ev_run(ev_handle_t evh)
{
    if (evh == NULL) {
        err("Invalid event handle");
        return EV_ERR;
    }
    struct epoll_event *evts = calloc(sizeof(struct epoll_event), evh->max_events);
    if (!evts) {
        err("Memory allocation failes for epoll events");
        return EV_ERR;
    }
    while(evh->stop == false) {
        int evtnums = epoll_wait(evh->epollfd, evts, evh->max_events, -1);
        if (evtnums < 0) {
            perr("epoll_wait");
            return EV_ERR;
        }
        dbg("epoll_wait received %d events", evtnums)
        for (int i = 0 ; i < evtnums ; i++) {
            ev_t *e = &evh->sub_events[evts[i].data.fd];
            if (!e->in_use) {
                err("Event received for non-added event (fd = %d)", evts[i].data.fd);
                continue;
            }
            if (e->fd != evts[i].data.fd) {
                err("fds do not match");
                continue;
            }
            if (evts[i].events & EPOLLIN) {
                process_ev_trig(e, EV_TRIG_READ);
            }
            if (evts[i].events & EPOLLIN) {
                process_ev_trig(e, EV_TRIG_WRITE);
            }
        }
    }
}

ev_res_t ev_deinit(ev_handle_t evh)
{

}

int main(void)
{return 0;}