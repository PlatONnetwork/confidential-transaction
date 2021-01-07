package com.platon.privacy.plaintext;

import com.alaya.crypto.Credentials;
import com.alaya.crypto.Hash;
import com.alaya.crypto.Keys;
import com.alaya.crypto.Sign;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Int128;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.google.common.primitives.Bytes;


import java.math.BigInteger;
import java.security.InvalidAlgorithmParameterException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.util.List;

public class Plaintext {
    public static long category = 1 << 24;
    public static long major = 1 << 16;
    public static long minor = 1 << 8;
    public static long currentVersion = category + major + minor;
    public static long transferProof = 1;
    public static long mintProof = 2;
    public static long burnProof = 3;
    public static long approveProof = 6;

    public static Plaintext.PlaintextProof createApprove(
            Plaintext.PlaintextApprove approve, Credentials admin) {
        byte[] proofData =
                RLPCodec.encode(
                        new Plaintext.PlaintextData(
                                Uint32.of(currentVersion + approveProof),
                                RLPCodec.encode(approve)));

        Sign.SignatureData signatureData = Sign.signMessage(proofData, admin.getEcKeyPair());

        Plaintext.PlaintextProof proof =
                new Plaintext.PlaintextProof(proofData, signToBytes(signatureData));
        return proof;
    }

    public static Plaintext.PlaintextInputNote[] createInputs(
            List<Plaintext.PlaintextNote> notes, WasmAddress spender, Credentials owner) {
        Plaintext.PlaintextInputNote[] pns = new Plaintext.PlaintextInputNote[notes.size()];
        for (int i = 0; i < notes.size(); i++) {
            Plaintext.PlaintextInputNote pi =
                    new Plaintext.PlaintextInputNote(
                            new WasmAddress(notes.get(i).owner.getAddress()),
                            notes.get(i).value,
                            notes.get(i).random,
                            null);
            Plaintext.PlaintextSpenderNote pn =
                    new Plaintext.PlaintextSpenderNote(pi.owner, pi.value, pi.random, spender);
            pi.signature = signToBytes(Sign.signMessage(RLPCodec.encode(pn), owner.getEcKeyPair()));
            pns[i] = pi;
        }
        return pns;
    }

    public static Plaintext.PlaintextProof createProof(
            Plaintext.PlaintextUTXO utxo, Credentials admin) {
        return createProof(utxo, admin, currentVersion);
    }

    public static Plaintext.PlaintextProof createProof(
            Plaintext.PlaintextUTXO utxo, Credentials admin, long version) {
        byte[] proofData =
                RLPCodec.encode(
                        new Plaintext.PlaintextData(
                                Uint32.of(version + transferProof), RLPCodec.encode(utxo)));

        Sign.SignatureData signatureData = Sign.signMessage(proofData, admin.getEcKeyPair());

        Plaintext.PlaintextProof proof =
                new Plaintext.PlaintextProof(proofData, signToBytes(signatureData));
        return proof;
    }

    public static byte[] signToBytes(Sign.SignatureData sig) {
        byte v = sig.getV()[0];
        v -= 27;
        byte[] data = Bytes.concat(sig.getR(), sig.getS(), new byte[] {v});
        return data;
    }

    public static Plaintext.PlaintextProof createMint(
            Plaintext.PlaintextMint mint, Credentials admin) {
        byte[] proofData =
                RLPCodec.encode(
                        new Plaintext.PlaintextData(
                                Uint32.of(currentVersion + mintProof), RLPCodec.encode(mint)));

        Sign.SignatureData signatureData = Sign.signMessage(proofData, admin.getEcKeyPair());

        Plaintext.PlaintextProof proof =
                new Plaintext.PlaintextProof(proofData, signToBytes(signatureData));
        return proof;
    }

    public static Plaintext.PlaintextProof createBurn(
            Plaintext.PlaintextBurn burn, Credentials admin) {
        byte[] proofData =
                RLPCodec.encode(
                        new Plaintext.PlaintextData(
                                Uint32.of(currentVersion + burnProof), RLPCodec.encode(burn)));

        Sign.SignatureData signatureData = Sign.signMessage(proofData, admin.getEcKeyPair());

        Plaintext.PlaintextProof proof =
                new Plaintext.PlaintextProof(proofData, signToBytes(signatureData));
        return proof;
    }

    public static class PlaintextProof {
        public byte[] data;
        public byte[] signature;

        public PlaintextProof(byte[] data, byte[] signature) {
            this.data = data;
            this.signature = signature;
        }
    }

    public static class PlaintextData {
        public Uint32 version;
        public byte[] data;

        public PlaintextData(Uint32 version, byte[] data) {
            this.version = version;
            this.data = data;
        }
    }

    public static class PlaintextNote {
        public WasmAddress owner;
        public BigInteger value;
        public byte[] random;

        public PlaintextNote(WasmAddress owner, BigInteger value)
                throws InvalidAlgorithmParameterException, NoSuchAlgorithmException,
                        NoSuchProviderException {
            this.owner = owner;
            this.value = value;
            this.random = random();
        }

        public PlaintextNote(WasmAddress owner, BigInteger value, byte[] random) {
            this.owner = owner;
            this.value = value;
            this.random = random;
        }

        public static byte[] random()
                throws InvalidAlgorithmParameterException, NoSuchAlgorithmException,
                        NoSuchProviderException {
            return Keys.createEcKeyPair().getPrivateKey().toByteArray();
        }
    }

    public static class PlaintextOutputNote {
        public WasmAddress owner;
        public BigInteger value;
        public byte[] random;
        public byte[] metaData;

        public PlaintextOutputNote(WasmAddress owner, BigInteger value, byte[] random) {
            this.owner = owner;
            this.value = value;
            this.random = random;
        }

        public PlaintextOutputNote(
                WasmAddress owner, BigInteger value, byte[] random, byte[] metaData) {
            this.owner = owner;
            this.value = value;
            this.random = random;
            this.metaData = metaData;
        }

        public byte[] hash() {
            return Hash.sha3(RLPCodec.encode(new PlaintextNote(owner, value, random)));
        }
    }

    public static class PlaintextSpenderNote {
        public WasmAddress owner;
        public BigInteger value;
        public byte[] random;
        public WasmAddress spender;

        public PlaintextSpenderNote(
                WasmAddress owner, BigInteger value, byte[] random, WasmAddress spender) {
            this.owner = owner;
            this.value = value;
            this.random = random;
            this.spender = spender;
        }
    }

    public static class PlaintextInputNote {
        public WasmAddress owner;
        public BigInteger value;
        public byte[] random;
        public byte[] signature;

        public PlaintextInputNote(
                WasmAddress owner, BigInteger value, byte[] random, byte[] signature) {
            this.owner = owner;
            this.value = value;
            this.random = random;
            this.signature = signature;
        }
    }

    public static class PlaintextUTXO {
        public PlaintextInputNote[] inputs;
        public PlaintextOutputNote[] outputs;
        public WasmAddress publicOwner;
        public Int128 publicValue;
        public byte[] metaData;
        public PlaintextUTXO(
                PlaintextInputNote[] inputs,
                PlaintextOutputNote[] outputs,
                WasmAddress publicOwner,
                Int128 publicValue,
                byte[] metaData) {
            this.inputs = inputs;
            this.outputs = outputs;
            this.publicOwner = publicOwner;
            this.publicValue = publicValue;
            this.metaData = metaData;
        }
    }

    public static class PlaintextApprove {
        public WasmAddress owner;
        public BigInteger value;
        public byte[] random;
        public byte[] signature;

        public PlaintextApprove(
                WasmAddress owner, BigInteger value, byte[] random, byte[] signature) {
            this.owner = owner;
            this.value = value;
            this.random = random;
            this.signature = signature;
        }
    }

    public static class PlaintextMint {
        public byte[] oldMintHash;

        public PlaintextOutputNote[] outputs;

        public PlaintextMint(byte[] oldMintHash, PlaintextOutputNote[] outputs) {
            this.oldMintHash = oldMintHash;
            this.outputs = outputs;
        }
    }

    public static class PlaintextBurn {
        public byte[] oldBurnHash;
        public PlaintextInputNote[] outputs;

        public PlaintextBurn(byte[] oldBurnHash, PlaintextInputNote[] outputs) {
            this.oldBurnHash = oldBurnHash;
            this.outputs = outputs;
        }
    }
}
