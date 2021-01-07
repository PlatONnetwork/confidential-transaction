package com.platon.privacy;


import com.alaya.rlp.wasm.datatypes.Int128;
import com.alaya.rlp.wasm.datatypes.Uint128;
import com.alaya.rlp.wasm.datatypes.WasmAddress;

public class Validator {
    public static class InputNotes {
        byte[] owner;
        byte[] noteHash;

        public InputNotes() {}

        public InputNotes(byte[] owner, byte[] noteHash) {
            this.owner = owner;
            this.noteHash = noteHash;
        }
    }

    public static class OutputNotes {
        byte[] owner;
        byte[] noteHash;
        byte[] metaData;

        public OutputNotes() {}

        public OutputNotes(byte[] owner, byte[] noteHash, byte[] metaData) {
            this.owner = owner;
            this.noteHash = noteHash;
            this.metaData = metaData;
        }
    }

    public static class TransferResult {
        InputNotes[] inputs;
        OutputNotes[] outputs;
        WasmAddress publicOwner;
        Int128 publicValue;
        byte[] sender;

        public TransferResult() {}

        public TransferResult(
                InputNotes[] inputs,
                OutputNotes[] outputs,
                WasmAddress publicOwner,
                Int128 publicValue,
                byte[] sender) {
            this.inputs = inputs;
            this.outputs = outputs;
            this.publicOwner = publicOwner;
            this.publicValue = publicValue;
            this.sender = sender;
        }
    }

    public static class ApproveResult {
        byte[] noteHash;
        byte[] shared_sign;
        byte[] sender;

        public ApproveResult() {}

        public ApproveResult(byte[] noteHash, byte[] shared_sign, byte[] sender) {
            this.noteHash = noteHash;
            this.shared_sign = shared_sign;
            this.sender = sender;
        }
    }

    public static class MintResult {
        byte[] oldMintHash;
        byte[] newMintHash;
        Uint128 totalMint;
        OutputNotes[] outputs;
        byte[] sender;

        public MintResult() {}

        public MintResult(
                byte[] oldMintHash, byte[] newMintHash, Uint128 totalMint, OutputNotes[] outputs, byte[] sender) {
            this.oldMintHash = oldMintHash;
            this.newMintHash = newMintHash;
            this.totalMint = totalMint;
            this.outputs = outputs;
            this.sender = sender;
        }
    }

    public static class BurnResult {
        byte[] oldBurnHash;
        byte[] newBurnHash;
        Uint128 totalBurn;
        InputNotes[] inputs;
        byte[] sender;

        public BurnResult() {}

        public BurnResult(
                byte[] oldBurnHash, byte[] newBurnHash, Uint128 totalBurn, InputNotes[] inputs, byte[] sender) {
            this.oldBurnHash = oldBurnHash;
            this.newBurnHash = newBurnHash;
            this.totalBurn = totalBurn;
            this.inputs = inputs;
            this.sender = sender;
        }
    }
}
