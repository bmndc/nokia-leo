/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * TCPServerSocket
 *
 * An interface to a server socket that can accept incoming connections for gaia apps.
 */

dictionary ServerSocketOptions {
  TCPSocketBinaryType binaryType = "string";
};

/**
 * Constructor for TCP server socket.
 *
 * @param address [optional] The address for socket to bind.
 *        ex:
 *        "0.0.0.0"   as all IPv4 interface.
 *        "127.0.0.1" as IPv4 localhost only.
 *        "::"        as all IPv6 interface.
 *        "::1"       as IPv6 localhost only.
 */
[Constructor(unsigned short port, optional ServerSocketOptions options,
             optional unsigned short backlog = 0, optional DOMString address = "0.0.0.0"),
 Func="mozilla::dom::TCPSocket::ShouldTCPSocketExist",
 Exposed=(Window,System)]
interface TCPServerSocket : EventTarget {
  /**
   * The port of this server socket object.
   */
  readonly attribute unsigned short localPort;

  /**
   * The "connect" event is dispatched when a client connection is accepted.
   * The event object will be a TCPServerSocketEvent containing a TCPSocket
   * instance, which is used for communication between client and server.
   */
  attribute EventHandler onconnect;

  /**
   * The "error" event will be dispatched when a listening server socket is
   * unexpectedly disconnected.
   */
  attribute EventHandler onerror;

  /**
   * Close the server socket.
   */
  void close();
};
