package com.platon.privacy.crypto;

import com.alaya.crypto.Hash;
import com.google.common.primitives.Bytes;
import org.bouncycastle.asn1.sec.SECNamedCurves;
import org.bouncycastle.asn1.x9.X9ECParameters;
import org.bouncycastle.crypto.InvalidCipherTextException;
import org.bouncycastle.crypto.agreement.ECDHBasicAgreement;
import org.bouncycastle.crypto.engines.AESEngine;
import org.bouncycastle.crypto.paddings.PaddedBufferedBlockCipher;
import org.bouncycastle.crypto.params.ECDomainParameters;
import org.bouncycastle.crypto.params.ECPrivateKeyParameters;
import org.bouncycastle.crypto.params.ECPublicKeyParameters;
import org.bouncycastle.crypto.params.KeyParameter;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.util.encoders.Hex;


import java.math.BigInteger;
import java.security.Security;
import java.util.Arrays;

public class SharedSecret {
    private static final String CURVE_NAME = "secp256k1";
    private static final X9ECParameters params = SECNamedCurves.getByName(CURVE_NAME);
    private static final ECDomainParameters CURVE =
            new ECDomainParameters(params.getCurve(), params.getG(), params.getN(), params.getH());

    private final byte[] privateKey;

    public SharedSecret(byte[] privateKey) {
        this.privateKey = privateKey;
    }

    public static SharedSecret generate(BigInteger sk, BigInteger pk) {
        ECPrivateKeyParameters privKeyP =
                new ECPrivateKeyParameters(new BigInteger(sk.toByteArray()), CURVE);
        byte[] prefix = Hex.decode("04");
        byte[] point = Bytes.concat(prefix, pk.toByteArray());

        ECPublicKeyParameters pubKeyP =
                new ECPublicKeyParameters(CURVE.getCurve().decodePoint(point), CURVE);

        ECDHBasicAgreement agreement = new ECDHBasicAgreement();
        agreement.init(privKeyP);
        System.out.println(agreement.calculateAgreement(pubKeyP).toString());
        return new SharedSecret(Hash.sha3(agreement.calculateAgreement(pubKeyP).toByteArray()));
    }

    public byte[] encryption(byte[] data) throws InvalidCipherTextException {
        Security.addProvider(new BouncyCastleProvider());
        PaddedBufferedBlockCipher encryptCipher = new PaddedBufferedBlockCipher(new AESEngine());
        encryptCipher.init(true, new KeyParameter(privateKey));
        byte[] out = new byte[1024];
        int len = encryptCipher.processBytes(data, 0, data.length, out, 0);
        len += encryptCipher.doFinal(out, len);

        return Arrays.copyOfRange(out, 0, len);
    }

    public byte[] decrypt(byte[] data) throws InvalidCipherTextException {
        PaddedBufferedBlockCipher decryptCipher = new PaddedBufferedBlockCipher(new AESEngine());
        decryptCipher.init(false, new KeyParameter(privateKey));
        byte[] plaintext = new byte[1024];

        int len = decryptCipher.processBytes(data, 0, data.length, plaintext, 0);
        len += decryptCipher.doFinal(plaintext, len);

        return Arrays.copyOfRange(plaintext, 0, len);
    }
}
