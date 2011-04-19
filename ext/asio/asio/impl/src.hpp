//
// impl/src.hpp
// ~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IMPL_SRC_HPP
#define ASIO_IMPL_SRC_HPP

#define ASIO_SOURCE

#include "asio/detail/config.hpp"

#if defined(ASIO_HEADER_ONLY)
# error Do not compile Asio library source with ASIO_HEADER_ONLY defined
#endif

#include "asio/impl/error.ipp"
#include "asio/impl/error_code.ipp"
#include "asio/impl/io_service.ipp"
#include "asio/impl/serial_port_base.ipp"
#include "asio/detail/impl/descriptor_ops.ipp"
#include "asio/detail/impl/dev_poll_reactor.ipp"
#include "asio/detail/impl/epoll_reactor.ipp"
#include "asio/detail/impl/eventfd_select_interrupter.ipp"
#include "asio/detail/impl/kqueue_reactor.ipp"
#include "asio/detail/impl/pipe_select_interrupter.ipp"
#include "asio/detail/impl/posix_event.ipp"
#include "asio/detail/impl/posix_mutex.ipp"
#include "asio/detail/impl/posix_thread.ipp"
#include "asio/detail/impl/posix_tss_ptr.ipp"
#include "asio/detail/impl/reactive_descriptor_service.ipp"
#include "asio/detail/impl/reactive_serial_port_service.ipp"
#include "asio/detail/impl/reactive_socket_service_base.ipp"
#include "asio/detail/impl/resolver_service_base.ipp"
#include "asio/detail/impl/select_reactor.ipp"
#include "asio/detail/impl/service_registry.ipp"
#include "asio/detail/impl/socket_ops.ipp"
#include "asio/detail/impl/socket_select_interrupter.ipp"
#include "asio/detail/impl/strand_service.ipp"
#include "asio/detail/impl/task_io_service.ipp"
#include "asio/detail/impl/throw_error.ipp"
#include "asio/detail/impl/timer_queue.ipp"
#include "asio/detail/impl/timer_queue_set.ipp"
#include "asio/detail/impl/win_iocp_handle_service.ipp"
#include "asio/detail/impl/win_iocp_io_service.ipp"
#include "asio/detail/impl/win_iocp_serial_port_service.ipp"
#include "asio/detail/impl/win_iocp_socket_service_base.ipp"
#include "asio/detail/impl/win_event.ipp"
#include "asio/detail/impl/win_mutex.ipp"
#include "asio/detail/impl/win_thread.ipp"
#include "asio/detail/impl/win_tss_ptr.ipp"
#include "asio/detail/impl/winsock_init.ipp"
#include "asio/ip/impl/address.ipp"
#include "asio/ip/impl/address_v4.ipp"
#include "asio/ip/impl/address_v6.ipp"
#include "asio/ip/impl/host_name.ipp"
#include "asio/ip/detail/impl/endpoint.ipp"
#include "asio/local/detail/impl/endpoint.ipp"

#endif // ASIO_IMPL_SRC_HPP
