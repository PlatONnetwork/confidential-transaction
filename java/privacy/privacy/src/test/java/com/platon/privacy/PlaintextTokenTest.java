package com.platon.privacy;

import com.platon.privacy.contracts.Acl;
import com.platon.privacy.contracts.Confidential_token;
import com.platon.privacy.crypto.SharedSecret;
import com.platon.privacy.plaintext.Plaintext;
import com.alaya.protocol.core.methods.response.Log;
import com.alaya.rlp.wasm.datatypes.*;
import com.platon.privacy.confidential.Confidential;
import com.platon.privacy.contracts.*;
import com.alaya.abi.wasm.WasmEventValues;
import com.alaya.crypto.Hash;
import com.alaya.crypto.Sign;
import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.tx.WasmContract;
import com.alaya.utils.Numeric;
import org.bouncycastle.util.encoders.Hex;
import org.junit.Assert;
import org.junit.jupiter.api.*;
import java.math.BigInteger;
import java.util.Arrays;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class PlaintextTokenTest {
    private static Deploy.Token token;
    private static byte[] random2;
    private static byte[] random7;
    private static byte[] random10;
    private static byte[] random12;
    private static byte[] random13;
    private static byte[] random15;
    private static byte[] random100;
    private static byte[] random101;

    @BeforeAll
    public static void testDeployContract() throws Exception {
        token = Deploy.deploy();
        random7 = Plaintext.PlaintextNote.random();

        random10 = Plaintext.PlaintextNote.random();
        random12 = Plaintext.PlaintextNote.random();
        random15 = Plaintext.PlaintextNote.random();
        random100 = Plaintext.PlaintextNote.random();
        random101 = Plaintext.PlaintextNote.random();
    }

    @Test
    @Order(1)
    public void testRegistry() throws Exception {
        Assertions.assertEquals(Deploy.tokenName, token.confidentialToken.Name().send());
        Assertions.assertEquals(Deploy.tokenSymbol, token.confidentialToken.Symbol().send());
        Assertions.assertEquals(Deploy.scalingFactor, token.confidentialToken.ScalingFactor().send());
        Assertions.assertEquals(Uint128.of(0), token.confidentialToken.TotalSupply().send());
    }

    @Test
    @Order(2)
    public void testTransferDeposit() throws Exception {
        System.out.println("admin:" + Web.admin.getAddress());
        System.out.println("user1:" + Web.user1.getAddress());
        Uint128 totalSupply = token.confidentialToken.TotalSupply().send();
        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 10, random10),
                            new Common.Note(Web.admin, 12, random12)
                        });

        System.out.println("outputs:" + Hex.toHexString(RLPCodec.encode(outputs)));

        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        new Plaintext.PlaintextInputNote[0],
                        outputs,
                        new WasmAddress(Web.admin.getAddress()),
                        Int128.of(-22),
                        null);

        Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        token.arc20.Approve(token.acl.GetTokenManager().send(), Uint128.of(220)).send();
        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                token.confidentialToken.TotalSupply().send().getValue().longValue(),
                totalSupply.getValue().longValue() + 22);
        System.out.println("gas used:" + receipt.getGasUsed());
        System.out.println("receipt size:" + receipt.getLogs().size());
    }

    @Test
    @Order(3)
    public void testTransfer() throws Exception {
        Uint128 totalSupply = token.confidentialToken.TotalSupply().send();
        Assertions.assertTrue(totalSupply.getValue().longValue() >= 22);
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    10,
                                    random10,
                                    new WasmAddress(Web.admin.getAddress())),
                            new Common.Note(
                                    Web.admin,
                                    12,
                                    random12,
                                    new WasmAddress(Web.admin.getAddress()))
                        });
        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 15, random15),
                            new Common.Note(Web.admin, 7, random7)
                        });
        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        inputs, outputs, new WasmAddress(BigInteger.ZERO), Int128.of(0), null);

        Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        System.out.println("gas used:" + receipt.getGasUsed());
        System.out.println("receipt size:" + receipt.getLogs().size());
    }

    @Test
    @Order(4)
    public void testApprove() throws Exception {
        SharedSecret sharedSecret =
                SharedSecret.generate(
                        Web.admin.getEcKeyPair().getPrivateKey(),
                        Web.user1.getEcKeyPair().getPublicKey());
        BigInteger fifteen = BigInteger.valueOf(15);
        byte[] noteHash =
                Hash.sha3(
                        RLPCodec.encode(
                                new Plaintext.PlaintextNote(
                                        new WasmAddress(Web.admin.getAddress()),
                                        fifteen,
                                        random15)));
        Plaintext.PlaintextSpenderNote pn =
                new Plaintext.PlaintextSpenderNote(
                        new WasmAddress(Web.admin.getAddress()),
                        fifteen,
                        random15,
                        new WasmAddress(Web.user1.getAddress()));
        byte[] signature =
                Plaintext.signToBytes(
                        Sign.signMessage(RLPCodec.encode(pn), Web.admin.getEcKeyPair()));

        byte[] sharedSign = sharedSecret.encryption(signature);

        Plaintext.PlaintextApprove approve =
                new Plaintext.PlaintextApprove(
                        new WasmAddress(Web.admin.getAddress()), fifteen, random15, sharedSign);

        Plaintext.PlaintextProof proof = Plaintext.createApprove(approve, Web.admin);
        TransactionReceipt receipt = token.confidentialToken.Approve(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK(), "approve failed");
        Assert.assertTrue(
                Arrays.equals(sharedSign, token.confidentialToken.GetApproval(noteHash).send()));

        byte[] decSign = sharedSecret.decrypt(sharedSign);

        Assert.assertTrue(Arrays.equals(signature, decSign));

        Plaintext.PlaintextInputNote[] inputs =
                new Plaintext.PlaintextInputNote[] {
                    new Plaintext.PlaintextInputNote(
                            new WasmAddress(Web.admin.getAddress()), fifteen, random15, decSign)
                };

        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.user1, 2, random2),
                            new Common.Note(Web.admin, 13, random13),
                        });
        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        inputs, outputs, new WasmAddress(BigInteger.ZERO), Int128.of(0), null);

        Plaintext.PlaintextProof utxoProof = Plaintext.createProof(utxo, Web.user1);
        receipt = token.confidentialToken.Transfer(RLPCodec.encode(utxoProof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
    }

    @Test
    @Order(5)
    public void testTransferWithdraw() throws Exception {
        Uint128 totalSupply = token.confidentialToken.TotalSupply().send();
        Assertions.assertTrue(totalSupply.getValue().longValue() >= 22);
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin, 7, random7, new WasmAddress(Web.admin.getAddress()))
                        });

        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        inputs,
                        new Plaintext.PlaintextOutputNote[0],
                        new WasmAddress(Web.admin.getAddress()),
                        Int128.of(7),
                        null);

        Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                token.confidentialToken.TotalSupply().send().getValue().longValue(),
                totalSupply.getValue().longValue() - 7);
        System.out.println("gas used:" + receipt.getGasUsed());
        System.out.println("receipt size:" + receipt.getLogs().size());
    }

    @Test
    @Order(6)
    public void testMint() throws Exception {
        Acl.Registry registry =
                token.acl.GetRegistry(new WasmAddress(token.confidentialToken.getContractAddress()))
                        .send();

        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 100, random100),
                            new Common.Note(Web.admin, 101, random101),
                        });
        Plaintext.PlaintextMint mint =
                new Plaintext.PlaintextMint(registry.baseClass.last_mint_hash, outputs);
        Plaintext.PlaintextProof proof = Plaintext.createMint(mint, Web.admin);
        long totalSupply = token.confidentialToken.TotalSupply().send().value().longValue();
        TransactionReceipt receipt = token.confidentialToken.Mint(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                totalSupply + 201, token.confidentialToken.TotalSupply().send().getValue().longValue());
    }

    @Test
    @Order(7)
    public void testBurn() throws Exception {
        Acl.Registry registry =
                token.acl.GetRegistry(new WasmAddress(token.confidentialToken.getContractAddress()))
                        .send();
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    100,
                                    random100,
                                    new WasmAddress(Web.admin.getAddress())),
                            new Common.Note(
                                    Web.admin,
                                    101,
                                    random101,
                                    new WasmAddress(Web.admin.getAddress()))
                        });

        Plaintext.PlaintextBurn burn =
                new Plaintext.PlaintextBurn(registry.baseClass.last_burn_hash, inputs);
        Plaintext.PlaintextProof proof = Plaintext.createBurn(burn, Web.admin);
        long totalSupply = token.confidentialToken.TotalSupply().send().value().longValue();
        TransactionReceipt receipt = token.confidentialToken.Burn(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                totalSupply - 201, token.confidentialToken.TotalSupply().send().getValue().longValue());
    }

    @Test
    @Order(8)
    public void testUpdateMetaData() throws Exception {
        byte[] noteHash =
                Hash.sha3(
                        RLPCodec.encode(
                                new Plaintext.PlaintextNote(
                                        new WasmAddress(Web.admin.getAddress()),
                                        BigInteger.valueOf(13),
                                        random13)));
        byte[] meta = org.bouncycastle.util.encoders.Hex.decode("1234");
        byte[] signature = Plaintext.signToBytes(Sign.signMessage(meta, Web.user1.getEcKeyPair()));

        TransactionReceipt receipt =
                token.confidentialToken.UpdateMetaData(noteHash, meta, signature).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(receipt.getLogs().size(), 1);
        WasmEventValues values =
                WasmContract.staticExtractEventParameters(
                        Confidential_token.METADATAEVENT_EVENT, receipt.getLogs().get(0));
        Assertions.assertArrayEquals(meta, (byte[]) values.getNonIndexedValues().get(0));
    }

    @Test
    @Order(9)
    public void testSupportProof() throws Exception {
        long transferProof = Plaintext.currentVersion + Plaintext.transferProof;
        Assertions.assertTrue(token.confidentialToken.SupportProof(Uint32.of(transferProof)).send());
        transferProof = Plaintext.currentVersion + (1 << 8) + Plaintext.transferProof;
        Assertions.assertFalse(token.confidentialToken.SupportProof(Uint32.of(transferProof)).send());
        transferProof = Plaintext.currentVersion - (1 << 8) + Plaintext.transferProof;
        Assertions.assertFalse(token.confidentialToken.SupportProof(Uint32.of(transferProof)).send());
    }
}
