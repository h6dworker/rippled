//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2015 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <BeastConfig.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/TestSuite.h>
#include <ripple/json/json_value.h>
#include <ripple/json/json_writer.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/handlers/WalletPropose.h>
#include <ripple/rpc/KeypairForSignature.h>

namespace ripple {

namespace RPC {

struct key_strings
{
    char const* account_id;
    char const* master_key;
    char const* master_seed;
    char const* master_seed_hex;
    char const* public_key;
    char const* public_key_hex;
    char const* secret_key_hex;
};

namespace common {
static char const* passphrase = "REINDEER FLOTILLA";
static char const* master_key = "SCAT BERN ISLE FOR ROIL BUS SOAK AQUA FREE FOR DRAM BRIG";
static char const* master_seed = "snMwVWs2hZzfDUF3p2tHZ3EgmyhFs";
static char const* master_seed_hex = "BE6A670A19B209E112146D0A7ED2AAD7";
}

static key_strings const secp256k1_strings =
{
    "r4Vtj2jrfmTVZGfSP3gH9hQPMqFPQFin8f",
    common::master_key,
    common::master_seed,
    common::master_seed_hex,
    "aBQxK2YFNqzmAaXNczYcjqDjfiKkLsJUizsr1UBf44RCF8FHdrmX",
    "038AAE247B2344B1837FBED8F57389C8C11774510A3F7D784F2A09F0CB6843236C",
    "1949ECD889EA71324BC7A30C8E81F4E93CB73EE19D59E9082111E78CC3DDABC2",
};

static key_strings const ed25519_strings =
{
    "r4qV6xTXerqaZav3MJfSY79ynmc1BSBev1",
    common::master_key,
    common::master_seed,
    common::master_seed_hex,
    "aKEQmgLMyZPMruJFejUuedp169LgW6DbJt1rej1DJ5hWUMH4pHJ7",
    "ED54C3F5BEDA8BD588B203D23A27398FAD9D20F88A974007D6994659CD7273FE1D",
    "77AAED2698D56D6676323629160F4EEF21CFD9EE3D0745CC78FA291461F98278",
};

class WalletPropose_test : public ripple::TestSuite
{
public:
    void testRandomWallet(boost::optional<std::string> const& keyType)
    {
        Json::Value params;
        if (keyType)
            params[jss::key_type] = *keyType;
        Json::Value result = walletPropose (params);

        expect (! contains_error (result));
        expect (result.isMember (jss::account_id));
        expect (result.isMember (jss::master_key));
        expect (result.isMember (jss::master_seed));
        expect (result.isMember (jss::master_seed_hex));
        expect (result.isMember (jss::public_key));
        expect (result.isMember (jss::public_key_hex));
        expect (result.isMember (jss::key_type));

        expectEquals (result[jss::key_type],
            params.isMember (jss::key_type) ? params[jss::key_type]
                                            : "secp256k1");

        std::string seed = result[jss::master_seed].asString();

        result = walletPropose (params);

        // We asked for two random seeds, so they shouldn't match.
        expect (result[jss::master_seed].asString() != seed, seed);
    }

    void testSecretWallet (Json::Value const& params, key_strings const& s)
    {
        Json::Value result = walletPropose (params);

        expect (! contains_error (result));
        expectEquals (result[jss::account_id], s.account_id);
        expectEquals (result[jss::master_key], s.master_key);
        expectEquals (result[jss::master_seed], s.master_seed);
        expectEquals (result[jss::master_seed_hex], s.master_seed_hex);
        expectEquals (result[jss::public_key], s.public_key);
        expectEquals (result[jss::public_key_hex], s.public_key_hex);
        expectEquals (result[jss::key_type],
            params.isMember (jss::key_type) ? params[jss::key_type]
                                            : "secp256k1");
    }

    void testSeed (boost::optional<std::string> const& keyType,
        key_strings const& strings)
    {
        testcase ("seed");

        Json::Value params;
        if (keyType)
            params[jss::key_type] = *keyType;
        params[jss::seed] = strings.master_seed;

        testSecretWallet (params, strings);
    }

    void testSeedHex (boost::optional<std::string> const& keyType,
        key_strings const& strings)
    {
        testcase ("seed_hex");

        Json::Value params;
        if (keyType)
            params[jss::key_type] = *keyType;
        params[jss::seed_hex] = strings.master_seed_hex;

        testSecretWallet (params, strings);
    }

    void testLegacyPassphrase (char const* value,
        boost::optional<std::string> const& keyType,
        key_strings const& strings)
    {
        Json::Value params;
        if (keyType)
            params[jss::key_type] = *keyType;
        params[jss::passphrase] = value;

        testSecretWallet (params, strings);
    }

    void testLegacyPassphrase(boost::optional<std::string> const& keyType,
        key_strings const& strings)
    {
        testcase ("passphrase");

        testLegacyPassphrase (common::passphrase, keyType, strings);
        testLegacyPassphrase (strings.master_key, keyType, strings);
        testLegacyPassphrase (strings.master_seed, keyType, strings);
        testLegacyPassphrase (strings.master_seed_hex, keyType, strings);
    }

    void testKeyType (boost::optional<std::string> const& keyType,
        key_strings const& strings)
    {
        testcase (keyType ? *keyType : "no key_type");

        testRandomWallet (keyType);
        testSeed (keyType, strings);
        testSeedHex (keyType, strings);
        testLegacyPassphrase (keyType, strings);

        Json::Value params;
        if (keyType)
            params[jss::key_type] = *keyType;
        params[jss::seed] = strings.master_seed;
        params[jss::seed_hex] = strings.master_seed_hex;

        // Secret fields are mutually exclusive.
        expect (contains_error (walletPropose (params)));
    }

    void run()
    {
        testKeyType (boost::none, secp256k1_strings);
        testKeyType (std::string("secp256k1"), secp256k1_strings);
        testKeyType (std::string("ed25519"), ed25519_strings);
    }
};

BEAST_DEFINE_TESTSUITE(WalletPropose,ripple_basics,ripple);

} // RPC
} // ripple
