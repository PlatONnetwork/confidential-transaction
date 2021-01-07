package com.platon.privacy;

import com.alaya.bech32.Bech32;
import com.alaya.crypto.Keys;
import com.alaya.parameters.NetworkParameters;
import com.platon.privacy.contracts.Plaintext_validator;
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
import org.junit.jupiter.api.*;


import java.math.BigInteger;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class PlaintextValidatorTest {
    private static Plaintext_validator validator;

    @BeforeAll
    public static void deployContract() throws Exception {
        validator =
                Plaintext_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.admin,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
    }

    @Test
    @Order(1)
    public void testTransfer() throws Exception {
        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 10), new Common.Note(Web.admin, 12)
                        });
        Plaintext.PlaintextUTXO utxo =
                new Plaintext.PlaintextUTXO(
                        new Plaintext.PlaintextInputNote[0],
                        outputs,
                        new WasmAddress(Web.admin.getAddress()),
                        Int128.of(-22),
                        null);

        Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        byte[] stream = validator.ValidateProof(RLPCodec.encode(proof)).send();
        Validator.TransferResult result =
                RLPCodec.decode(
                        stream, Validator.TransferResult.class, Web.chainManager.getChainId());
        Assertions.assertEquals(2, result.outputs.length);
        Assertions.assertEquals(-22, result.publicValue.value.longValue());
        Assertions.assertEquals(new WasmAddress(Web.admin.getAddress()), result.publicOwner);
        for (int i = 0; i < result.outputs.length; i++) {
            Assertions.assertArrayEquals(outputs[i].hash(), result.outputs[i].noteHash);
            Assertions.assertArrayEquals(outputs[i].owner.getValue(), result.outputs[i].owner);
        }

        utxo =
                new Plaintext.PlaintextUTXO(
                        new Plaintext.PlaintextInputNote[0],
                        outputs,
                        new WasmAddress(Web.admin.getAddress()),
                        Int128.of(-23),
                        null);

        final Plaintext.PlaintextProof wrongProof = Plaintext.createProof(utxo, Web.admin);
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    validator.ValidateProof(RLPCodec.encode(wrongProof)).send();
                });
    }

    @Test
    @Order(2)
    public void testApprove() throws Exception {
        SharedSecret sharedSecret =
                SharedSecret.generate(
                        Web.admin.getEcKeyPair().getPrivateKey(),
                        Web.user1.getEcKeyPair().getPublicKey());
        BigInteger fifteen = BigInteger.valueOf(15);
        byte[] random = Plaintext.PlaintextNote.random();
        byte[] noteHash =
                Hash.sha3(
                        RLPCodec.encode(
                                new Plaintext.PlaintextNote(
                                        new WasmAddress(Web.admin.getAddress()), fifteen, random)));
        Plaintext.PlaintextSpenderNote pn =
                new Plaintext.PlaintextSpenderNote(
                        new WasmAddress(Web.admin.getAddress()),
                        fifteen,
                        random,
                        new WasmAddress(Web.user1.getAddress()));
        byte[] signature =
                Plaintext.signToBytes(
                        Sign.signMessage(RLPCodec.encode(pn), Web.admin.getEcKeyPair()));

        byte[] sharedSign = sharedSecret.encryption(signature);

        Plaintext.PlaintextApprove approve =
                new Plaintext.PlaintextApprove(
                        new WasmAddress(Web.admin.getAddress()), fifteen, random, sharedSign);

        Plaintext.PlaintextProof proof = Plaintext.createApprove(approve, Web.admin);
        byte[] stream = validator.ValidateProof(RLPCodec.encode(proof)).send();
        Validator.ApproveResult result =
                RLPCodec.decode(
                        stream, Validator.ApproveResult.class, Web.chainManager.getChainId());
        Assertions.assertArrayEquals(sharedSign, result.shared_sign);
        Assertions.assertArrayEquals(noteHash, result.noteHash);
    }

    @Test
    @Order(3)
    public void testMint() throws Exception {
        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 100), new Common.Note(Web.admin, 101),
                        });

        Plaintext.PlaintextMint mint = new Plaintext.PlaintextMint(new byte[32], outputs);
        Plaintext.PlaintextProof proof = Plaintext.createMint(mint, Web.admin);

        byte[] stream = validator.ValidateProof(RLPCodec.encode(proof)).send();
        Validator.MintResult result =
                RLPCodec.decode(stream, Validator.MintResult.class, Web.chainManager.getChainId());
        Assertions.assertEquals(2, result.outputs.length);
        Assertions.assertEquals(201, result.totalMint.getValue().longValue());

        mint = new Plaintext.PlaintextMint(new byte[32], outputs);
        final Plaintext.PlaintextProof wrongProof = Plaintext.createMint(mint, Web.user1);
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    validator.ValidateProof(RLPCodec.encode(wrongProof)).send();
                });
    }

    @Test
    @Order(4)
    public void testBurn() throws Exception {
        byte[] random = Plaintext.PlaintextNote.random();
        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    100,
                                    random,
                                    new WasmAddress(Web.admin.getAddress())),
                            new Common.Note(
                                    Web.admin, 101, random, new WasmAddress(Web.admin.getAddress()))
                        });

        Plaintext.PlaintextBurn burn = new Plaintext.PlaintextBurn(new byte[32], inputs);
        Plaintext.PlaintextProof proof = Plaintext.createBurn(burn, Web.admin);
        byte[] stream = validator.ValidateProof(RLPCodec.encode(proof)).send();
        Validator.BurnResult result =
                RLPCodec.decode(stream, Validator.BurnResult.class, Web.chainManager.getChainId());
        Assertions.assertEquals(2, result.inputs.length);
        Assertions.assertEquals(201, result.totalBurn.getValue().longValue());

        Plaintext.PlaintextProof wrongProof = Plaintext.createBurn(burn, Web.user1);
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    validator.ValidateProof(RLPCodec.encode(wrongProof)).send();
                });
    }

    @Test
    @Order(5)
    public void testSignature() throws Exception {
        byte[] data = new byte[] {1};
        byte[] hash = Hash.sha3(data);
        byte[] signature = Plaintext.signToBytes(Sign.signMessage(data, Web.admin.getEcKeyPair()));
        System.out.println(Hex.toHexString(signature));
        BigInteger key =
                Sign.signedMessageToKey(hash, Sign.signMessage(hash, Web.admin.getEcKeyPair()));
        System.out.println(
                Bech32.addressEncode(
                        NetworkParameters.getHrp(Web.chainManager.getChainId()),
                        "0x" + Keys.getAddress(key)));
        byte[] owner = new WasmAddress(Web.admin.getAddress()).getValue();
        Assertions.assertTrue(validator.ValidateSignature(owner, hash, signature).send());
        Assertions.assertFalse(validator.ValidateSignature(hash, hash, signature).send());
        Assertions.assertFalse(validator.ValidateSignature(owner, hash, owner).send());
    }

    @Test
    @Order(6)
    public void testSupportProof() throws Exception {
        Assertions.assertFalse(validator.SupportProof(Uint32.of(Plaintext.currentVersion)).send());
        Assertions.assertFalse(
                validator.SupportProof(Uint32.of(Plaintext.currentVersion + 100)).send());
        Assertions.assertTrue(
                validator.SupportProof(Uint32.of(Plaintext.currentVersion + Plaintext.burnProof))
                        .send());
        Assertions.assertFalse(
                validator.SupportProof(Uint32.of(Plaintext.currentVersion + (1 << 16))).send());
    }
}
