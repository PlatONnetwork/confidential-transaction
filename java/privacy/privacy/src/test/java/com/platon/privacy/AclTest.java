package com.platon.privacy;

import com.alaya.bech32.Bech32;
import com.alaya.crypto.Keys;
import com.alaya.parameters.NetworkParameters;
import com.alaya.protocol.exceptions.TransactionException;
import com.platon.privacy.contracts.Acl;
import com.platon.privacy.contracts.Token_manager;
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
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Order;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;

public class AclTest {
    public static String tokenName = "ABC";
    public static String tokenSymbol = "abc";
    public static Uint128 scalingFactor = Uint128.of(10);
    public static Uint128 totalSupply = Uint128.of(100000);
    public static Uint8 decimals = Uint8.of(10);
    private static Acl acl;

    @BeforeAll
    public static void deployContract() throws Exception {
        String validatorAddr = Deploy.deployPlaintextValidator().getContractAddress();
        String storageAddr = Deploy.deployStorage().getContractAddress();
        Token_manager tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        acl = Deploy.deployAcl(false, "", tokenManager);
        acl.CreateValidator(Uint32.of(Plaintext.currentVersion), new WasmAddress(validatorAddr), "")
                .send();
        acl.CreateStorage(Uint32.of(Plaintext.currentVersion), new WasmAddress(storageAddr), "")
                .send();
    }

    @Test
    @Order(1)
    public void testCreateRegistry() throws Exception {
        TransactionReceipt receipt =
                acl.CreateRegistry(
                                Uint32.of(Plaintext.currentVersion),
                                Uint32.of(Plaintext.currentVersion),
                                Uint128.of(0),
                                new WasmAddress(BigInteger.ZERO),
                                false)
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    acl.CreateRegistry(
                                    Uint32.of(0),
                                    Uint32.of(0),
                                    Uint128.of(0),
                                    new WasmAddress(BigInteger.ZERO),
                                    false)
                            .send();
                });
    }

    @Test
    @Order(2)
    public void testValidateSignature() throws Exception {
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
        Assertions.assertTrue(acl.ValidateSignature(owner, hash, signature).send());
        Assertions.assertFalse(acl.ValidateSignature(hash, hash, signature).send());
        Assertions.assertFalse(acl.ValidateSignature(owner, hash, owner).send());
    }

    @Test
    @Order(3)
    public void testValidateProof() throws Exception {
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
        byte[] stream = acl.ValidateProof(RLPCodec.encode(proof)).send();
        Validator.TransferResult result =
                RLPCodec.decode(
                        stream, Validator.TransferResult.class, Web.chainManager.getChainId());
        Assertions.assertEquals(2, result.outputs.length);
        Assertions.assertEquals(-22, result.publicValue.value.longValue());
        Assertions.assertEquals(new WasmAddress(Web.admin.getAddress()), result.publicOwner);

        Acl user1Acl =
                Acl.load(
                        acl.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user1,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    user1Acl.ValidateProof(RLPCodec.encode(proof)).send();
                });
    }

    @Test
    @Order(4)
    public void testRegistry() throws Exception {
        String validatorAddr = Deploy.deployPlaintextValidator().getContractAddress();
        String storageAddr = Deploy.deployStorage().getContractAddress();
        Token_manager tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        Acl acl = Deploy.deployAcl(false, "", tokenManager);
        acl.CreateValidator(Uint32.of(Plaintext.currentVersion), new WasmAddress(validatorAddr), "")
                .send();
        acl.CreateStorage(Uint32.of(Plaintext.currentVersion), new WasmAddress(storageAddr), "")
                .send();
        TransactionReceipt receipt =
                acl.CreateRegistry(
                                Uint32.of(Plaintext.currentVersion),
                                Uint32.of(Plaintext.currentVersion),
                                Uint128.of(0),
                                new WasmAddress(BigInteger.ZERO),
                                false)
                        .send();
        Acl.Registry registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertEquals(registry.baseClass.total_supply.value.longValue(), 0);
        Assertions.assertEquals(registry.baseClass.token_addr, new WasmAddress(BigInteger.ZERO));
        Assertions.assertEquals(registry.baseClass.can_mint_burn, false);
        Assertions.assertArrayEquals(registry.baseClass.last_burn_hash, new byte[32]);
        Assertions.assertArrayEquals(registry.baseClass.last_mint_hash, new byte[32]);
        Assertions.assertEquals(registry.baseClass.scaling_factor.value.longValue(), 0);
    }

    @Test
    @Order(5)
    public void testTransfer() throws Exception {
        Token_manager tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        Deploy.Token token = Deploy.deployAcl(true, false, false, "", tokenManager);
        Acl acl = token.acl;
        Plaintext.PlaintextOutputNote[] outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 10), new Common.Note(Web.admin, 12)
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
        TransactionReceipt receipt = acl.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                acl.GetRegistry(new WasmAddress(Web.admin.getAddress()))
                        .send()
                        .baseClass
                        .total_supply
                        .value
                        .longValue(),
                22);

        Plaintext.PlaintextInputNote[] inputs =
                Common.createInputs(
                        new Common.Note[] {
                            new Common.Note(
                                    Web.admin,
                                    10,
                                    outputs[0].random,
                                    new WasmAddress(Web.admin.getAddress())),
                            new Common.Note(
                                    Web.admin,
                                    12,
                                    outputs[1].random,
                                    new WasmAddress(Web.admin.getAddress()))
                        });
        outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 15), new Common.Note(Web.admin, 7)
                        });
        utxo =
                new Plaintext.PlaintextUTXO(
                        inputs, outputs, new WasmAddress(BigInteger.ZERO), Int128.of(0), null);

        proof = Plaintext.createProof(utxo, Web.admin);
        receipt = acl.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        System.out.println("gas used:" + receipt.getGasUsed());
        System.out.println("receipt size:" + receipt.getLogs().size());
    }

    @Test
    @Order(6)
    public void testNoArc20NoMint() throws Exception {
        Token_manager tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        Acl acl = Deploy.deployAcl(false, false, false, "", tokenManager).acl;
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

        final Plaintext.PlaintextProof proof = Plaintext.createProof(utxo, Web.admin);
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    acl.Transfer(RLPCodec.encode(proof)).send();
                });

        outputs =
                Common.createOutputs(
                        new Common.Note[] {
                            new Common.Note(Web.admin, 100), new Common.Note(Web.admin, 101),
                        });
        Acl.Registry registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();

        Plaintext.PlaintextMint mint =
                new Plaintext.PlaintextMint(registry.baseClass.last_mint_hash, outputs);
        Plaintext.PlaintextProof mintProof = Plaintext.createMint(mint, Web.admin);
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    acl.Mint(RLPCodec.encode(mintProof)).send();
                });
    }

    @Test
    @Order(7)
    public void testMintBurn() throws Exception {
        Token_manager tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        Acl acl = Deploy.deployAcl(false, true, false, "", tokenManager).acl;
        Acl.Registry registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();

        Common.Note[] notes =
                new Common.Note[] {
                    new Common.Note(Web.admin, 100), new Common.Note(Web.admin, 101),
                };
        Plaintext.PlaintextOutputNote[] outputs = Common.createOutputs(notes);
        Plaintext.PlaintextMint mint =
                new Plaintext.PlaintextMint(registry.baseClass.last_mint_hash, outputs);
        Plaintext.PlaintextProof proof = Plaintext.createMint(mint, Web.admin);

        TransactionReceipt receipt = acl.Mint(RLPCodec.encode(proof)).send();
        Acl.Registry mintRegistry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                registry.baseClass.total_supply.value.longValue() + 201,
                mintRegistry.baseClass.total_supply.getValue().longValue());

        registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        for (int i = 0; i < notes.length; i++) {
            notes[i].spender = new WasmAddress(Web.admin.getAddress());
        }
        Plaintext.PlaintextInputNote[] inputs = Common.createInputs(notes);

        Plaintext.PlaintextBurn burn =
                new Plaintext.PlaintextBurn(registry.baseClass.last_burn_hash, inputs);
        proof = Plaintext.createBurn(burn, Web.admin);
        receipt = acl.Burn(RLPCodec.encode(proof)).send();
        Acl.Registry burnRegistry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                registry.baseClass.total_supply.value.longValue() - 201,
                burnRegistry.baseClass.total_supply.getValue().longValue());
    }
}
