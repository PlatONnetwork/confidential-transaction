package com.platon.privacy.confidential;

import com.alaya.crypto.Credentials;
import com.alaya.crypto.Sign;
import com.alaya.parameters.NetworkParameters;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.Uint64;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.google.common.primitives.Bytes;

import java.math.BigInteger;
import java.util.LinkedList;
import java.util.List;

public class Confidential {
    public static long name = 1 << 24;
    public static long major = 1 << 16;
    public static long minor = 1 << 8;
    public static long currentVersion = name + major + minor;

    public static long minorUpdateVersion = name + major + (2 << 8);
    public static long majorUpdateVersion = name + (2 << 16) + minor;

    static {
        System.out.println(minorUpdateVersion);
        System.out.println(majorUpdateVersion);
    }


    public byte[] viewSk;
    public byte[] viewPk;
    public byte[] spendSk;
    public byte[] spendPk;
    public long chainId;

    public static class Keypair {
        public byte[] privateKey;
        public byte[] publicKey;
    }

    public static Keypair CreateKeypair() throws Exception {
        byte[] bytes = ConfidentialTxJNA.create_keypair(new byte[10], 0);
        return RLPCodec.decode(bytes, Keypair.class, NetworkParameters.CurrentNetwork.getChainId());
    }

    public Confidential(long chainId) throws Exception {
        this.chainId = chainId;
        Keypair viewKeyPair = CreateKeypair();
        this.viewSk = viewKeyPair.privateKey;
        this.viewPk = viewKeyPair.publicKey;
        Keypair spendKeyPair = CreateKeypair();
        this.spendSk = spendKeyPair.privateKey;
        this.spendPk = spendKeyPair.publicKey;
    }

    public static class ConfidentialInput {
        public byte[] ephemeral_pk;
        public byte[] sign_pk;
        public BigInteger quatity;
        public byte[] blinding;
        public byte[] view_sk;
        public byte[] spend_sk;

        public ConfidentialInput(){}

        public ConfidentialInput(Confidential one, BigInteger quatity, byte[] blinding, byte[] ephemeral_pk, byte[] sign_pk){
            this.ephemeral_pk = ephemeral_pk;
            this.sign_pk = sign_pk;
            this.quatity = quatity;
            this.blinding = blinding;
            this.view_sk = one.viewSk;
            this.spend_sk = one.spendSk;
        }

        public ConfidentialInput(ConfidentialOutputNote output, byte[] view_sk, byte[] spend_sk) throws Exception {
            byte[] value = ConfidentialTxJNA.decrypt_note(output.cipherValue, view_sk);
            Confidential.PlainValue plainValue = RLPCodec.decode(value, Confidential.PlainValue.class, NetworkParameters.CurrentNetwork.getChainId());
            this.ephemeral_pk = output.ephemeralPk;
            this.sign_pk = output.signPk;
            this.quatity = plainValue.quatity.value;
            this.blinding = plainValue.blinding;
            this.view_sk = view_sk;
            this.spend_sk = spend_sk;
        }

        public byte[] getEphemeral_pk() {
            return ephemeral_pk;
        }

        public void setEphemeral_pk(byte[] ephemeral_pk) {
            this.ephemeral_pk = ephemeral_pk;
        }

        public byte[] getSign_pk() {
            return sign_pk;
        }

        public void setSign_pk(byte[] sign_pk) {
            this.sign_pk = sign_pk;
        }

        public BigInteger getQuatity() {
            return quatity;
        }

        ;

        public void setQuatity(BigInteger quatity) {
            this.quatity = quatity;
        }

        public byte[] getBlinding() {
            return blinding;
        }

        public void setBlinding(byte[] blinding) {
            this.blinding = blinding;
        }

        public byte[] getView_sk() {
            return view_sk;
        }

        public void setView_sk(byte[] view_sk) {
            this.view_sk = view_sk;
        }

        public byte[] getSpend_sk() {
            return spend_sk;
        }

        public void setSpend_sk(byte[] spend_sk) {
            this.spend_sk = spend_sk;
        }
    }

    public static class ConfidentialOutput {
        public BigInteger quatity;
        public byte[] view_pk;
        public byte[] spend_pk;

        public ConfidentialOutput(Confidential one, BigInteger quatity) {
            this.quatity = quatity;
            this.view_pk = one.viewPk;
            this.spend_pk = one.spendPk;
        }
        public ConfidentialOutput(byte[] view_pk, byte[] spend_pk, BigInteger quatity) {
            this.quatity = quatity;
            this.view_pk = view_pk;
            this.spend_pk = spend_pk;
        }



        public BigInteger getQuatity() {
            return quatity;
        }

        public void setQuatity(BigInteger quatity) {
            this.quatity = quatity;
        }

        public byte[] getView_pk() {
            return view_pk;
        }

        public void setView_pk(byte[] view_pk) {
            this.view_pk = view_pk;
        }

        public byte[] getSpend_pk() {
            return spend_pk;
        }

        public void setSpend_pk(byte[] spend_pk) {
            this.spend_pk = spend_pk;
        }
    }


    public static class ConfidentialTXType {
        public static final int TRANSFER = 1;
        public static final int MINT = 2;
        public static final int BURN = 3;
        public static final int DEPOSIT = 4;
        public static final int WITHDRAW = 5;
        public static final int APPROVE = 6;
    }

    public static class Transfer {
        public int tx_type;
        public List<ConfidentialInput> input;
        public List<ConfidentialOutput> output;
        public byte[] authorized_address;

        public Transfer(List<ConfidentialInput> input, List<ConfidentialOutput> output, byte[] authorized_address) {
            this.tx_type = ConfidentialTXType.TRANSFER;
            this.input = input;
            this.output = output;
            this.authorized_address = authorized_address;
        }

        public Transfer() {
            this.input = new LinkedList<>();
            this.output = new LinkedList<>();
            this.tx_type = ConfidentialTXType.TRANSFER;
        }

        public int getTx_type() {
            return tx_type;
        }

        public void setTx_type(int tx_type) {
            this.tx_type = tx_type;
        }

        public List<ConfidentialInput> getInput() {
            return input;
        }

        public void setInput(List<ConfidentialInput> input) {
            this.input = input;
        }

        public List<ConfidentialOutput> getOutput() {
            return output;
        }

        public void setOutput(List<ConfidentialInput> output) {
            this.input = output;
        }

        public byte[] getAuthorized_address() {
            return authorized_address;
        }

        public void setAuthorized_address(byte[] authorized_address) {
            this.authorized_address = authorized_address;
        }
    }


    public static class Mint {
        public int tx_type;
        public List<ConfidentialOutput> output;
        public byte[] authorized_address;

        public Mint(List<ConfidentialOutput> output, byte[] authorized_address) {
            this.tx_type = ConfidentialTXType.MINT;
            this.output = output;
            this.authorized_address = authorized_address;
        }

        public Mint() {
            this.output = new LinkedList<>();
            this.tx_type = ConfidentialTXType.MINT;
        }

        public int getTx_type() {
            return tx_type;
        }

        public void setTx_type(int tx_type) {
            this.tx_type = tx_type;
        }

        public List<ConfidentialOutput> getOutput() {
            return output;
        }

        public void setOutput(List<ConfidentialOutput> output) {
            this.output = output;
        }

        public byte[] getAuthorized_address() {
            return authorized_address;
        }

        public void setAuthorized_address(byte[] authorized_address) {
            this.authorized_address = authorized_address;
        }
    }

    public static class Burn {
        public int tx_type;
        public List<ConfidentialInput> input;
        public byte[] authorized_address;

        public Burn() {
            this.input = new LinkedList<>();
            this.tx_type = ConfidentialTXType.BURN;
        }

        public int getTx_type() {
            return tx_type;
        }

        public void setTx_type(int tx_type) {
            this.tx_type = tx_type;
        }

        public List<ConfidentialInput> getInput() {
            return input;
        }

        public void setInput(List<ConfidentialInput> input) {
            this.input = input;
        }

        public byte[] getAuthorized_address() {
            return authorized_address;
        }

        public void setAuthorized_address(byte[] authorized_address) {
            this.authorized_address = authorized_address;
        }
    }

    public static class Deposit {
        public int tx_type;
        public List<ConfidentialInput> input;
        public List<ConfidentialOutput> output;
        public byte[] authorized_address;

        public Deposit(List<ConfidentialInput> input, List<ConfidentialOutput> output, byte[] authorized_address) {
            this.tx_type = ConfidentialTXType.DEPOSIT;
            this.input = input;
            this.output = output;
            this.authorized_address = authorized_address;
        }

        public Deposit() {
            this.input = new LinkedList<>();
            this.output = new LinkedList<>();
            this.tx_type = ConfidentialTXType.DEPOSIT;
        }

        public int getTx_type() {
            return tx_type;
        }

        public void setTx_type(int tx_type) {
            this.tx_type = tx_type;
        }

        public List<ConfidentialInput> getInput() {
            return input;
        }

        public void setInput(List<ConfidentialInput> input) {
            this.input = input;
        }

        public List<ConfidentialOutput> getOutput() {
            return output;
        }

        public void setOutput(List<ConfidentialOutput> output) {
            this.output = output;
        }

        public byte[] getAuthorized_address() {
            return authorized_address;
        }

        public void setAuthorized_address(byte[] authorized_address) {
            this.authorized_address = authorized_address;
        }
    }

    public static class Withdraw {
        public int tx_type;
        public List<ConfidentialInput> input;
        public List<ConfidentialOutput> output;
        public byte[] authorized_address;

        public Withdraw(List<ConfidentialInput> input, List<ConfidentialOutput> output, byte[] authorized_address) {
            this.tx_type = ConfidentialTXType.WITHDRAW;
            this.input = input;
            this.output = output;
            this.authorized_address = authorized_address;
        }

        public Withdraw() {
            this.input = new LinkedList<>();
            this.output = new LinkedList<>();
            this.tx_type = ConfidentialTXType.WITHDRAW;
        }

        public int getTx_type() {
            return tx_type;
        }

        public void setTx_type(int tx_type) {
            this.tx_type = tx_type;
        }

        public List<ConfidentialInput> getInput() {
            return input;
        }

        public void setInput(List<ConfidentialInput> input) {
            this.input = input;
        }

        public List<ConfidentialOutput> getOutput() {
            return output;
        }

        public void setOutput(List<ConfidentialOutput> output) {
            this.output = output;
        }

        public byte[] getAuthorized_address() {
            return authorized_address;
        }

        public void setAuthorized_address(byte[] authorized_address) {
            this.authorized_address = authorized_address;
        }
    }


    public static class TransferExtra {
        public WasmAddress publicOwner; // desposit withdraw arc20 address
        public byte[] depositSignature; // deposit signature
        public List<byte[]> metaData; // per output note meta data

        public TransferExtra() {
            this.publicOwner = new WasmAddress(new byte[20]);
            this.metaData = new LinkedList<>();
        }
        public TransferExtra(WasmAddress publicOwner) {
            this.publicOwner = publicOwner;
            this.metaData = new LinkedList<>();
        }
    }

    public static class MintExtra {
        public byte[] oldMintHash; // last mint hash
        public List<byte[]> metaData; // per output meta hash

        public MintExtra() {
            this.metaData = new LinkedList<>();
        }
    }

    public static class BurnExtra {
        public byte[] oldBurnHash; // last burn hash
    }

    public static class ConfidentialData {
        public Uint32 version;
        public byte[] confidentialTx;
        public byte[] extraData;
        public ConfidentialData(){}
        public ConfidentialData(int type, byte[] confidentialTx, byte[] extraData) {
            this.version = Uint32.of(Confidential.currentVersion + type);
            this.confidentialTx = confidentialTx;
            this.extraData = extraData;
        }
    }

    public static class ConfidentialProof {
        public byte[] data;
        public byte[] signature;

        public ConfidentialProof(){}
        public ConfidentialProof(byte[] data, Credentials sender) {
            this.data = data;
            this.signature = signature(data, sender);
        }

        public byte[] signature(byte[] data, Credentials sender) {
            Sign.SignatureData signatureData = Sign.signMessage(data, sender.getEcKeyPair());
            byte v = signatureData.getV()[0];
            v -= 27;
            return Bytes.concat(signatureData.getR(), signatureData.getS(), new byte[]{v});
        }

    }

    public static class EncryptedOwner {
        public byte[] ephemeralPk;
        public byte[] signPk;
    }

    public static class PlainValue {
        public Uint64 quatity;
        public byte[] blinding;
    }

    public static class ConfidentialInputNote {
        public byte[] nodeId;
        public byte[] ephemeralPk;
        public byte[] signPk;
        public byte[] token;
    }

    public static class ConfidentialOutputNote {
        public byte[] nodeId;
        public byte[] ephemeralPk;
        public byte[] signPk;
        public byte[] token;
        public byte[] cipherValue;
    }

    public static class ConfidentialUTXO {
        public ConfidentialUTXO() {
            this.inputs = new LinkedList<>();
            this.outputs = new LinkedList<>();
        }

        public byte txType;
        public List<ConfidentialInputNote> inputs;
        public List<ConfidentialOutputNote> outputs;
        public Uint64 publicValue;
        public WasmAddress authorizedAddress;
    }

    public static class OutputInfo extends ConfidentialInput {
        public String owner;
        public String hash;
        public boolean hasCreateNoteEvent;

        public OutputInfo(Confidential one, BigInteger quatity, byte[] blinding, byte[] ephemeral_pk, byte[] sign_pk, String owner, String hash, boolean hasCreateNoteEvent) {
            super(one, quatity, blinding, ephemeral_pk, sign_pk);
            this.owner = owner;
            this.hash = hash;
            this.hasCreateNoteEvent = hasCreateNoteEvent;
        }
    }

    public static class MetaDataInfo {
        public String hash;
        public String data;

        public MetaDataInfo(String hash, String data) {
            this.hash = hash;
            this.data = data;
        }
    }

}
