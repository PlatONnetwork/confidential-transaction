package com.platon.privacy;

import com.alaya.abi.wasm.WasmFunctionEncoder;
import com.platon.privacy.contracts.Acl;
import com.platon.privacy.contracts.Multisig;
import com.platon.privacy.contracts.Registry;
import com.platon.privacy.contracts.Token_manager;
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
import org.apache.commons.codec.Charsets;
import org.bouncycastle.util.Arrays;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

public class AclDeployTest {

    private static Multisig multisig;
    private static WasmAddress[] owners;

    public static String encodeEventName(String name) {
        byte[] data = name.getBytes(Charsets.UTF_8);
        return Numeric.toHexString(Arrays.concatenate(new byte[32 - data.length], data));
    }

    @BeforeAll
    public static void deployMultisig() throws Exception {
        owners = new WasmAddress[3];
        owners[0] = new WasmAddress(Web.admin.getAddress());
        owners[1] = new WasmAddress(Web.user1.getAddress());
        owners[2] = new WasmAddress(Web.user2.getAddress());
        multisig =
                Multisig.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                owners,
                                Uint16.of(2))
                        .send();
    }

    @Test
    public void deployAclTest() throws Exception {
        Token_manager tokenManagerTemplate =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        Acl aclTemplate = ConfidentialDeploy.deployAcl(false, "", tokenManagerTemplate);

        Registry registry =
                Registry.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        List<Object> parameters = new ArrayList<>();
        parameters.add(WasmFunctionEncoder.fnvOne64Hash("init"));
        parameters.add(new WasmAddress(registry.getContractAddress()));
        parameters.add(new WasmAddress(tokenManagerTemplate.getContractAddress()));
        byte[] data = RLPCodec.encode(parameters);

        TransactionReceipt receipt =
                multisig.CloneContract(
                                new WasmAddress(aclTemplate.getContractAddress()),
                                data,
                                Uint32.of(200000))
                        .send();
        Assertions.assertTrue(receipt.isStatusOK());

        Multisig.Transaction[] transactions = multisig.GetPendingTransactions().send();
        Assertions.assertTrue(transactions.length == 1);

        Multisig signMultiSig =
                Multisig.load(
                        multisig.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user1,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        TransactionReceipt signReceipt = signMultiSig.SignTransaction(Uint32.of(1)).send();
        Assertions.assertTrue(signReceipt.isStatusOK());

        WasmAddress aclAddress = registry.GetContractAddress("acl").send();

        String cloneEvent = AclDeployTest.encodeEventName(Multisig.MULTISIGCLONE_EVENT.getName());
        signReceipt
                .getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(cloneEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Multisig.MULTISIGCLONE_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                WasmAddress newAddress =
                                        (WasmAddress) values.getNonIndexedValues().get(0);
                                System.out.println(newAddress.getAddress());
                                Assertions.assertEquals(aclAddress, newAddress);
                            }
                        });
    }
}
