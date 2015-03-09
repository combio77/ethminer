/*
 This file is part of cpp-ethereum.
 
 cpp-ethereum is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 cpp-ethereum is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
 */
/** @file RLPXHandshake.h
 * @author Alex Leverington <nessence@gmail.com>
 * @date 2015
 */


#pragma once

#include <memory>
#include <libdevcrypto/Common.h>
#include <libdevcrypto/ECDHE.h>
#include "RLPxFrameIO.h"
#include "Common.h"
namespace ba = boost::asio;
namespace bi = boost::asio::ip;

namespace dev
{
namespace p2p
{

/**
 * @brief Setup inbound or outbound connection for communication over RLPXFrameIO.
 * RLPx Spec: https://github.com/ethereum/devp2p/blob/master/rlpx.md#encrypted-handshake
 *
 * @todo Implement StartSession transition via lambda which is passed to constructor.
 *
 * Thread Safety
 * Distinct Objects: Safe.
 * Shared objects: Unsafe.
 */
class RLPXHandshake: public std::enable_shared_from_this<RLPXHandshake>
{
	friend class RLPXFrameIO;
	
	/// Sequential states of handshake
	enum State
	{
		Error = -1,
		New,
		AckAuth,
		WriteHello,
		ReadHello,
		StartSession
	};
	
public:
	/// Setup incoming connection.
	RLPXHandshake(Host* _host, std::shared_ptr<RLPXSocket> const& _socket): m_host(_host), m_originated(false), m_socket(_socket) { crypto::Nonce::get().ref().copyTo(m_nonce.ref()); }
	
	/// Setup outbound connection.
	RLPXHandshake(Host* _host, std::shared_ptr<RLPXSocket> const& _socket, NodeId _remote): m_host(_host), m_remote(_remote), m_originated(true), m_socket(_socket) { crypto::Nonce::get().ref().copyTo(m_nonce.ref()); }

	~RLPXHandshake() {}

	/// Start handshake.
	void start() { transition(); }

	void cancel() { m_nextState = Error; }
	
protected:
	/// Write Auth message to socket and transitions to AckAuth.
	void writeAuth();
	
	/// Reads Auth message from socket and transitions to AckAuth.
	void readAuth();
	
	/// Write Ack message to socket and transitions to WriteHello.
	void writeAck();
	
	/// Reads Auth message from socket and transitions to WriteHello.
	void readAck();
	
	/// Closes connection and ends transitions.
	void error();
	
	/// Performs transition for m_nextState.
	void transition(boost::system::error_code _ech = boost::system::error_code());

	State m_nextState = New;			///< Current or expected state of transition.
	
	Host* m_host;					///< Host which provides m_alias, protocolVersion(), m_clientVersion, caps(), and TCP listenPort().
	
	/// Node id of remote host for socket.
	NodeId m_remote;					///< Public address of remote host.
	bool m_originated = false;		///< True if connection is outbound.
	
	/// Buffers for encoded and decoded handshake phases
	bytes m_auth;					///< Plaintext of egress or ingress Auth message.
	bytes m_authCipher;				///< Ciphertext of egress or ingress Auth message.
	bytes m_ack;						///< Plaintext of egress or ingress Ack message.
	bytes m_ackCipher;				///< Ciphertext of egress or ingress Ack message.
	bytes m_handshakeOutBuffer;		///< Frame buffer for egress Hello packet.
	bytes m_handshakeInBuffer;		///< Frame buffer for ingress Hello packet.
	
	crypto::ECDHE m_ecdhe;			///< Ephemeral ECDH secret and agreement.
	h256 m_nonce;					///< Nonce generated by this host for handshake.
	
	Public m_remoteEphemeral;			///< Remote ephemeral public key.
	h256 m_remoteNonce;				///< Nonce generated by remote host for handshake.
	
	/// Used to read and write RLPx encrypted frames for last step of handshake authentication.
	/// Passed onto Host which will take ownership.
	RLPXFrameIO* m_io = nullptr;
	
	std::shared_ptr<RLPXSocket> m_socket;	///< Socket.
};
	
}
}