/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2024-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#include "Constants.hpp"
#include "RuntimeEnvironment.hpp"
#include "Credential.hpp"
#include "Capability.hpp"
#include "Tag.hpp"
#include "CertificateOfMembership.hpp"
#include "CertificateOfOwnership.hpp"
#include "Revocation.hpp"
#include "Network.hpp"
#include "Topology.hpp"

// These are compile-time asserts to make sure temporary marshal buffers here and
// also in NtworkConfig.cpp are always large enough to marshal all credential types.
#if ZT_TAG_MARSHAL_SIZE_MAX > ZT_BUF_MEM_SIZE
#error ZT_TAG_MARSHAL_SIZE_MAX exceeds maximum buffer size
#endif
#if ZT_CAPABILITY_MARSHAL_SIZE_MAX > ZT_BUF_MEM_SIZE
#error ZT_CAPABILITY_MARSHAL_SIZE_MAX exceeds maximum buffer size
#endif
#if ZT_REVOCATION_MARSHAL_SIZE_MAX > ZT_BUF_MEM_SIZE
#error ZT_REVOCATION_MARSHAL_SIZE_MAX exceeds maximum buffer size
#endif
#if ZT_CERTIFICATEOFOWNERSHIP_MARSHAL_SIZE_MAX > ZT_BUF_MEM_SIZE
#error ZT_CERTIFICATEOFOWNERSHIP_MARSHAL_SIZE_MAX exceeds maximum buffer size
#endif
#if ZT_CERTIFICATEOFMEMBERSHIP_MARSHAL_SIZE_MAX > ZT_BUF_MEM_SIZE
#error ZT_CERTIFICATEOFMEMBERSHIP_MARSHAL_SIZE_MAX exceeds maximum buffer size
#endif

namespace ZeroTier {

template<typename CRED>
static ZT_INLINE Credential::VerifyResult _credVerify(const RuntimeEnvironment *RR,void *tPtr,CRED credential)
{
	uint8_t tmp[ZT_BUF_MEM_SIZE + 16];

	const Address signedBy(credential.signer());
	const uint64_t networkId = credential.networkId();
	if ((!signedBy)||(signedBy != Network::controllerFor(networkId)))
		return Credential::VERIFY_BAD_SIGNATURE;

	const SharedPtr<Peer> peer(RR->topology->peer(tPtr,signedBy));
	if (!peer)
		return Credential::VERIFY_NEED_IDENTITY;

	try {
		int l = credential.marshal(tmp,true);
		if (l <= 0)
			return Credential::VERIFY_BAD_SIGNATURE;
		return (peer->identity().verify(tmp,(unsigned int)l,credential.signature(),credential.signatureLength()) ? Credential::VERIFY_OK : Credential::VERIFY_BAD_SIGNATURE);
	} catch ( ... ) {}

	return Credential::VERIFY_BAD_SIGNATURE;
}

Credential::VerifyResult Credential::_verify(const RuntimeEnvironment *const RR,void *tPtr,const Revocation &credential) const { return _credVerify(RR,tPtr,credential); }
Credential::VerifyResult Credential::_verify(const RuntimeEnvironment *const RR,void *tPtr,const Tag &credential) const { return _credVerify(RR,tPtr,credential); }
Credential::VerifyResult Credential::_verify(const RuntimeEnvironment *const RR,void *tPtr,const Capability &credential) const { return _credVerify(RR,tPtr,credential); }
Credential::VerifyResult Credential::_verify(const RuntimeEnvironment *const RR,void *tPtr,const CertificateOfOwnership &credential) const { return _credVerify(RR,tPtr,credential); }

Credential::VerifyResult Credential::_verify(const RuntimeEnvironment *const RR,void *tPtr,const CertificateOfMembership &credential) const
{
	// Sanity check network ID.
	if ((!credential._signedBy)||(credential._signedBy != Network::controllerFor(credential._networkId)))
		return Credential::VERIFY_BAD_SIGNATURE;

	// If we don't know the peer, get its identity. This shouldn't happen here but should be handled.
	const SharedPtr<Peer> peer(RR->topology->peer(tPtr,credential._signedBy));
	if (!peer)
		return Credential::VERIFY_NEED_IDENTITY;

	// Now verify the controller's signature.
	uint64_t buf[ZT_CERTIFICATEOFMEMBERSHIP_MARSHAL_SIZE_MAX / 8];
	const unsigned int bufSize = credential._fillSigningBuf(buf);
	return peer->identity().verify(buf,bufSize,credential._signature,credential._signatureLength) ? Credential::VERIFY_OK : Credential::VERIFY_BAD_SIGNATURE;
}

} // namespace ZeroTier
