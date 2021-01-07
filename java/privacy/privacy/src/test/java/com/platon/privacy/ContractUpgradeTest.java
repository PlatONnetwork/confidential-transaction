package com.platon.privacy;

import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Uint128;
import com.alaya.rlp.wasm.datatypes.Uint16;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.platon.privacy.contracts.Acl;
import com.platon.privacy.contracts.Plaintext_validator_upgrade_test;
import com.platon.privacy.contracts.Storage;
import com.platon.privacy.contracts.Token_manager;
import com.platon.privacy.plaintext.Plaintext;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;

public class ContractUpgradeTest {

    private Acl deployAcl() throws Exception {
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

        TransactionReceipt receipt =
                acl.CreateStorage(
                                Uint32.of(Plaintext.currentVersion),
                                new WasmAddress(storageAddr),
                                "hello")
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());
        receipt =
                acl.CreateValidator(
                                Uint32.of(Plaintext.currentVersion),
                                new WasmAddress(validatorAddr),
                                "world")
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());

        receipt =
                acl.CreateRegistry(
                                Uint32.of(Plaintext.currentVersion),
                                Uint32.of(Plaintext.currentVersion),
                                Uint128.of(0),
                                new WasmAddress(BigInteger.ZERO),
                                true)
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());
        return acl;
    }

    @Test
    public void testUpgradeValidator() throws Exception {
        Acl acl = deployAcl();

        Plaintext_validator_upgrade_test upgrade =
                Plaintext_validator_upgrade_test.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();

        Uint32 newVersion = Uint32.of(Plaintext.currentVersion + Plaintext.minor);
        TransactionReceipt receipt =
                acl.UpdateValidatorVersion(
                                newVersion, new WasmAddress(upgrade.getContractAddress()), "world")
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());

        Acl.Registry registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        receipt = acl.UpdateValidator(newVersion).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Acl.Registry newRegistry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertNotEquals(
                registry.validator_addr.toString(), newRegistry.validator_addr.toString());
        registry = newRegistry;

        upgrade =
                Plaintext_validator_upgrade_test.load(
                        registry.validator_addr.getAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        upgrade.SetVersion(Uint16.of(100), Uint16.of(100)).send();
        Assertions.assertEquals(100, upgrade.Major().send().value.longValue());
        Assertions.assertEquals(100, upgrade.Minor().send().value.longValue());

        receipt = acl.UpdateValidator(newVersion).send();
        Assertions.assertTrue(receipt.isStatusOK());
        newRegistry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertNotEquals(
                registry.validator_addr.toString(), newRegistry.validator_addr.toString());

        final Plaintext_validator_upgrade_test oldValidator =
                Plaintext_validator_upgrade_test.load(
                        registry.validator_addr.getAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    oldValidator.Major().send();
                });
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    oldValidator.Minor().send();
                });

        upgrade =
                Plaintext_validator_upgrade_test.load(
                        newRegistry.validator_addr.getAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        Assertions.assertEquals(100, upgrade.Major().send().value.longValue());
        Assertions.assertEquals(100, upgrade.Minor().send().value.longValue());

        System.out.println("update gas:" + receipt.getGasUsed());
    }

    @Test
    public void testStorageMigrate() throws Exception {
        Acl acl = deployAcl();

        // deploy upgrade storage contract
        Uint32 newVersion = Uint32.of(Plaintext.currentVersion + Plaintext.minor);
        Storage upgrade =
                Storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        TransactionReceipt receipt =
                acl.UpdateStorageVersion(
                                newVersion, new WasmAddress(upgrade.getContractAddress()), "world")
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());

        Acl.Registry registry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();

        Common.Note[] notes =
                new Common.Note[] {
                    new Common.Note(Web.admin, 100), new Common.Note(Web.admin, 101),
                };
        Plaintext.PlaintextOutputNote[] outputs = Common.createOutputs(notes);
        Plaintext.PlaintextMint mint =
                new Plaintext.PlaintextMint(registry.baseClass.last_mint_hash, outputs);
        Plaintext.PlaintextProof proof = Plaintext.createMint(mint, Web.admin);
        receipt = acl.Mint(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Storage oldStorage =
                Storage.load(
                        registry.storage_addr.getAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        for (int i = 0; i < outputs.length; i++) {
            Assertions.assertArrayEquals(
                    oldStorage.GetNote(outputs[i].hash()).send().owner,
                    new WasmAddress(Web.admin.getAddress()).getValue());
        }

        receipt = acl.UpdateStorage(newVersion).send();
        Assertions.assertTrue(receipt.isStatusOK());

        Acl.Registry newRegistry = acl.GetRegistry(new WasmAddress(Web.admin.getAddress())).send();
        Assertions.assertNotEquals(
                registry.storage_addr.toString(), newRegistry.storage_addr.toString());

        Storage storage =
                Storage.load(
                        newRegistry.storage_addr.getAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        for (int i = 0; i < outputs.length; i++) {
            Assertions.assertArrayEquals(
                    storage.GetNote(outputs[i].hash()).send().owner,
                    new WasmAddress(Web.admin.getAddress()).getValue());
        }
    }
}
