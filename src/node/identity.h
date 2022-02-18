// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once

#include "ccf/crypto/curve.h"
#include "crypto/certs.h"
#include "crypto/openssl/key_pair.h"

#include <openssl/crypto.h>
#include <string>
#include <vector>

namespace ccf
{
  enum class IdentityType
  {
    REPLICATED,
    SPLIT
  };

  struct NetworkIdentity
  {
    crypto::Pem priv_key;
    crypto::Pem cert;
    std::optional<IdentityType> type;

    bool operator==(const NetworkIdentity& other) const
    {
      return cert == other.cert && priv_key == other.priv_key &&
        type == other.type;
    }

    NetworkIdentity() : type(IdentityType::REPLICATED) {}
    NetworkIdentity(IdentityType type) : type(type) {}

    virtual crypto::Pem issue_certificate(
      const std::string& valid_from, size_t validity_period_days)
    {
      return {};
    }

    virtual void set_certificate(const crypto::Pem& certificate) {}

    virtual ~NetworkIdentity() {}
  };

  class ReplicatedNetworkIdentity : public NetworkIdentity
  {
  public:
    static constexpr auto subject_name = "CN=CCF Network";

    ReplicatedNetworkIdentity() : NetworkIdentity(IdentityType::REPLICATED) {}

    ReplicatedNetworkIdentity(
      crypto::CurveID curve_id,
      const std::string& valid_from,
      size_t validity_period_days) :
      NetworkIdentity(IdentityType::REPLICATED)
    {
      auto identity_key_pair =
        std::make_shared<crypto::KeyPair_OpenSSL>(curve_id);
      priv_key = identity_key_pair->private_key_pem();

      cert = crypto::create_self_signed_cert(
        identity_key_pair,
        subject_name,
        {} /* SAN */,
        valid_from,
        validity_period_days);
    }

    ReplicatedNetworkIdentity(const NetworkIdentity& other) :
      NetworkIdentity(IdentityType::REPLICATED)
    {
      if (type != other.type)
      {
        throw std::runtime_error("invalid identity type conversion");
      }
      priv_key = other.priv_key;
      cert = other.cert;
    }

    virtual crypto::Pem issue_certificate(
      const std::string& valid_from, size_t validity_period_days) override
    {
      auto identity_key_pair =
        std::make_shared<crypto::KeyPair_OpenSSL>(priv_key);

      return crypto::create_self_signed_cert(
        identity_key_pair,
        subject_name,
        {} /* SAN */,
        valid_from,
        validity_period_days);
    }

    virtual void set_certificate(const crypto::Pem& new_cert) override
    {
      cert = new_cert;
    }

    ~ReplicatedNetworkIdentity() override
    {
      OPENSSL_cleanse(priv_key.data(), priv_key.size());
    }
  };

  class SplitNetworkIdentity : public NetworkIdentity
  {
  public:
    SplitNetworkIdentity() : NetworkIdentity(IdentityType::SPLIT) {}

    SplitNetworkIdentity(const NetworkIdentity& other) :
      NetworkIdentity(IdentityType::SPLIT)
    {
      if (type != other.type)
      {
        throw std::runtime_error("invalid identity type conversion");
      }
      priv_key = {};
      cert = other.cert;
    }
  };
}