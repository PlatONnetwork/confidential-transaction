package com.platon.privacy;

import com.alaya.crypto.Credentials;
import com.alaya.crypto.Sign;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.platon.privacy.plaintext.Plaintext;
import org.apache.commons.codec.binary.Hex;

import java.math.BigInteger;
import java.security.InvalidAlgorithmParameterException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;

public class Common {
    public static Plaintext.PlaintextInputNote[] createInputs(Note[] notes) {
        Plaintext.PlaintextInputNote[] pns = new Plaintext.PlaintextInputNote[notes.length];
        for (int i = 0; i < notes.length; i++) {
            Plaintext.PlaintextInputNote pi =
                    new Plaintext.PlaintextInputNote(
                            new WasmAddress(notes[i].owner.getAddress()),
                            BigInteger.valueOf(notes[i].value),
                            notes[i].random,
                            null);
            Plaintext.PlaintextSpenderNote pn =
                    new Plaintext.PlaintextSpenderNote(
                            pi.owner, pi.value, pi.random, notes[i].spender);
            System.out.println(
                    "stream:"
                            + Hex.encodeHexString(RLPCodec.encode(pn))
                            + "  random:"
                            + Hex.encodeHexString(pi.random));
            pi.signature =
                    Plaintext.signToBytes(
                            Sign.signMessage(RLPCodec.encode(pn), notes[i].owner.getEcKeyPair()));
            pns[i] = pi;
        }
        return pns;
    }

    public static Plaintext.PlaintextOutputNote[] createOutputs(Note[] notes) {
        Plaintext.PlaintextOutputNote[] pns = new Plaintext.PlaintextOutputNote[notes.length];
        for (int i = 0; i < notes.length; i++) {
            Plaintext.PlaintextOutputNote pi =
                    new Plaintext.PlaintextOutputNote(
                            new WasmAddress(notes[i].owner.getAddress()),
                            BigInteger.valueOf(notes[i].value),
                            notes[i].random);
            pns[i] = pi;
        }
        return pns;
    }

    public static class Note {
        Credentials owner;
        long value;
        byte[] random;
        WasmAddress spender;

        public Note(Credentials owner, long value)
                throws InvalidAlgorithmParameterException, NoSuchAlgorithmException,
                        NoSuchProviderException {
            this.owner = owner;
            this.value = value;
            this.random = Plaintext.PlaintextNote.random();
        }

        public Note(Credentials owner, long value, byte[] random) {
            this.owner = owner;
            this.value = value;
            this.random = random;
        }

        public Note(Credentials owner, long value, byte[] random, WasmAddress spender) {
            this.owner = owner;
            this.value = value;
            this.random = random;
            this.spender = spender;
        }
    }
}
