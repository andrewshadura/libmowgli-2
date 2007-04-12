/*
 * libmowgli: A collection of useful routines for programming.
 * mowgli_ioevent.c: Portable I/O event layer.
 *
 * Copyright (c) 2007 William Pitcock <nenolod -at- sacredspiral.co.uk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "mowgli.h"

#ifdef HAVE_EPOLL_CTL
# include <sys/epoll.h>
#endif

#define MOWGLI_POLLRDNORM		0x01
#define MOWGLI_POLLWRNORM		0x02
#define MOWGLI_POLLHUP			0x04
#define MOWGLI_POLLERR			0x08

mowgli_ioevent_handle_t *mowgli_ioevent_create(void)
{
	mowgli_ioevent_handle_t *self = mowgli_alloc(sizeof(mowgli_ioevent_handle_t));

#ifdef HAVE_EPOLL_CTL
	self->impldata = (void *) epoll_create(FD_SETSIZE);
#endif

	return self;
}

void mowgli_ioevent_destroy(mowgli_ioevent_handle_t *self)
{
	return_if_fail(self != NULL);

#ifdef HAVE_EPOLL_CTL
	close((int) self->impldata);
#endif

	mowgli_free(self);
}

int mowgli_ioevent_get(mowgli_ioevent_handle_t *self, mowgli_ioevent_t *buf, size_t bufsize, unsigned int delay)
{
#ifdef HAVE_EPOLL_CTL
	struct epoll_event events[bufsize];
	int ret, iter;

	ret = epoll_wait((int) self->impldata, events, bufsize, delay);

	for (iter = 0; iter < ret; iter++)
	{
		buf[iter].ev_status = 0;
		buf[iter].ev_object = events[iter].data.fd;
		buf[iter].ev_opaque = events[iter].data.ptr;
		buf[iter].ev_source = MOWGLI_SOURCE_FD;

		if (events[iter].events & EPOLLIN)
			buf[iter].ev_status |= MOWGLI_POLLRDNORM;

		if (events[iter].events & EPOLLOUT)
			buf[iter].ev_status |= MOWGLI_POLLWRNORM;

		if (events[iter].events & EPOLLHUP)
			buf[iter].ev_status = MOWGLI_POLLHUP;

		if (events[iter].events & EPOLLERR)
			buf[iter].ev_status = MOWGLI_POLLERR;
	}
#endif

	return ret;
}

void mowgli_ioevent_associate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object, unsigned int flags, void *opaque)
{
	if (source != MOWGLI_SOURCE_FD)
		return;

#ifdef HAVE_EPOLL_CTL
	{
		struct epoll_event ep_event = {};
		int events = EPOLLONESHOT;

		if (flags & MOWGLI_POLLRDNORM)
			events |= EPOLLIN;

		if (flags & MOWGLI_POLLWRNORM)
			events |= EPOLLOUT;

		ep_event.events = events;
		ep_event.data.ptr = opaque;

		epoll_ctl((int) self->impldata, EPOLL_CTL_ADD, object, &ep_event);
	}
#endif
}

void mowgli_ioevent_dissociate(mowgli_ioevent_handle_t *self, mowgli_ioevent_source_t source, int object)
{
	if (source != MOWGLI_SOURCE_FD)
		return;

#ifdef HAVE_EPOLL_CTL
	{
		struct epoll_event ep_event = {};

		epoll_ctl((int) self->impldata, EPOLL_CTL_DEL, object, &ep_event);
	}
#endif
}
