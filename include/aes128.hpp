#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>

using namespace CryptoPP;

byte key[AES::DEFAULT_KEYLENGTH] = { … };
byte plain[16] = { … };
byte cipher[16];

ECB_Mode<AES>::Encryption encryptor;
encryptor.SetKey(key, sizeof(key));

ArraySource(plain, sizeof(plain), true,
            new StreamTransformationFilter(
                encryptor,
                new ArraySink(cipher,
                              sizeof(cipher))) // StreamTransformationFilter
);                                             // ArraySource
