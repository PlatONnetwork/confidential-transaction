package com.platon.privacy;


import java.math.BigInteger;

public class Utxo {
    public static class Note {
        public byte[] hash;
        public byte[] owner;
        public BigInteger value;

        public Note(byte[] hash, byte[] owner) {
            this.hash = hash;
            this.owner = owner;
        }

        public Note(byte[] hash, byte[] owner, BigInteger value) {
            this.hash = hash;
            this.owner = owner;
            this.value = value;
        }


    }
}


