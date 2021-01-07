package com.platon.privacy;

import com.alaya.abi.wasm.WasmEventValues;
import com.alaya.crypto.Hash;
import com.alaya.crypto.Sign;
import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Int128;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.alaya.tx.WasmContract;
import com.alaya.utils.Numeric;
import com.platon.privacy.contracts.Acl;
import com.platon.privacy.contracts.Confidential_token;
import com.platon.privacy.crypto.SharedSecret;
import com.platon.privacy.plaintext.Plaintext;

import org.apache.commons.codec.Charsets;
import org.bouncycastle.util.Arrays;
import org.junit.Assert;
import org.junit.jupiter.api.*;


import java.math.BigInteger;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class PlaintextTokenEventTest {
    private static final String metaEventName =
            encodeEventName(Confidential_token.METADATAEVENT_EVENT.getName());
    private static Plaintext.PlaintextOutputNote[] outputs;
    private static Deploy.Token token;

    @BeforeAll
    public static void deploy() throws Exception {
        token = Deploy.deploy();
    }

    private static String encodeEventName(String name) {
        byte[] data = name.getBytes(Charsets.UTF_8);
        return Numeric.toHexString(Arrays.concatenate(new byte[32 - data.length], data));
    }

    @Test
    @Order(1)
    public void testMetaDataEvent() throws Exception {
        // test mint
        Acl.Registry registry =
                token.acl.GetRegistry(new WasmAddress(token.confidentialToken.getContractAddress()))
                        .send();
        outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 100), new Common.Note(Web.admin, 101),
                        });
        byte[] metaData = Numeric.hexStringToByteArray("0x123456");
        for (int i = 0; i < outputs.length; i++) {
            outputs[i].metaData = metaData;
        }

        Plaintext.PlaintextMint mint =
                new Plaintext.PlaintextMint(registry.baseClass.last_mint_hash, outputs);
        Plaintext.PlaintextProof proof = Plaintext.createMint(mint, Web.admin);
        long totalSupply = token.confidentialToken.TotalSupply().send().value().longValue();
        TransactionReceipt receipt = token.confidentialToken.Mint(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                totalSupply + 201, token.confidentialToken.TotalSupply().send().getValue().longValue());

        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(metaEventName)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.METADATAEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());
                                byte[] value = (byte[]) values.getNonIndexedValues().get(0);
                                Assertions.assertArrayEquals(metaData, value);
                            }
                        });

        // test transfer
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    outputs[0].value.longValue(),
                                    outputs[0].random,
                                    new WasmAddress(Web.admin.getAddress())),
                        });
        outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, outputs[0].value.longValue() / 2),
                            new Common.Note(Web.admin, outputs[0].value.longValue() / 2)
                        });
        for (int i = 0; i < outputs.length; i++) {
            outputs[i].metaData = metaData;
        }
        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        inputs, outputs, new WasmAddress(BigInteger.ZERO), Int128.of(0), null);

        proof = Plaintext.createProof(utxo, Web.admin);
        receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(metaEventName)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.METADATAEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());
                                byte[] value = (byte[]) values.getNonIndexedValues().get(0);
                                Assertions.assertArrayEquals(metaData, value);
                            }
                        });
    }

    @Test
    @Order(2)
    public void testApproveEvent() throws Exception {
        SharedSecret sharedSecret =
                SharedSecret.generate(
                        Web.admin.getEcKeyPair().getPrivateKey(),
                        Web.user1.getEcKeyPair().getPublicKey());
        BigInteger fifteen = BigInteger.valueOf(15);
        byte[] noteHash =
                Hash.sha3(
                        RLPCodec.encode(
                                new Plaintext.PlaintextNote(
                                        outputs[0].owner, outputs[0].value, outputs[0].random)));
        Plaintext.PlaintextSpenderNote pn =
                new Plaintext.PlaintextSpenderNote(
                        outputs[0].owner,
                        outputs[0].value,
                        outputs[0].random,
                        new WasmAddress(Web.user1.getAddress()));
        byte[] signature =
                Plaintext.signToBytes(
                        Sign.signMessage(RLPCodec.encode(pn), Web.admin.getEcKeyPair()));

        byte[] sharedSign = sharedSecret.encryption(signature);

        Plaintext.PlaintextApprove approve =
                new Plaintext.PlaintextApprove(
                        outputs[0].owner, outputs[0].value, outputs[0].random, sharedSign);

        Plaintext.PlaintextProof proof = Plaintext.createApprove(approve, Web.admin);
        TransactionReceipt receipt = token.confidentialToken.Approve(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK(), "approve failed");
        Assert.assertTrue(
                java.util.Arrays.equals(
                        sharedSign, token.confidentialToken.GetApproval(noteHash).send()));
        String approveEventName = encodeEventName(Confidential_token.APPROVEEVENT_EVENT.getName());
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(approveEventName)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.APPROVEEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());
                                byte[] value = (byte[]) values.getNonIndexedValues().get(0);
                                Assertions.assertArrayEquals(sharedSign, value);
                            }
                        });
    }

    @Test
    @Order(3)
    public void testCreateEvent() throws Exception {
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    outputs[0].value.longValue(),
                                    outputs[0].random,
                                    new WasmAddress(Web.admin.getAddress())),
                        });
        outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, outputs[0].value.longValue() / 2),
                            new Common.Note(Web.admin, outputs[0].value.longValue() / 2)
                        });

        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        inputs, outputs, new WasmAddress(BigInteger.ZERO), Int128.of(0), null);

        Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        String createNoteEventName = encodeEventName(Confidential_token.CREATENOTEEVENT_EVENT.getName());
        String destroyNoteEventName =
                encodeEventName(Confidential_token.DESTROYNOTEEVENT_EVENT.getName());

        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(createNoteEventName)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.CREATENOTEEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());
                                byte[] value = (byte[]) values.getNonIndexedValues().get(0);
                                Assertions.assertArrayEquals(outputs[0].owner.getValue(), value);
                            } else if (name.equals(destroyNoteEventName)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.DESTROYNOTEEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());
                                byte[] value = (byte[]) values.getNonIndexedValues().get(0);
                                Assertions.assertArrayEquals(outputs[0].owner.getValue(), value);
                            }
                        });
    }
}
