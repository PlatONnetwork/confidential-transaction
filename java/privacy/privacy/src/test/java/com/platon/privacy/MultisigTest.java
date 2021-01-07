package com.platon.privacy;


import com.alaya.crypto.Credentials;
import com.alaya.protocol.Web3j;
import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.wasm.datatypes.Uint16;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.Uint64;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.alaya.tx.gas.GasProvider;
import com.platon.privacy.contracts.*;
import org.junit.jupiter.api.*;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class MultisigTest {
    private static Multisig multisig;
    private static WasmAddress[] owners;
    private static Simple_contract simpleContract;

    private static class MultisigInner extends Multisig {

        public MultisigInner(
                String contractAddress,
                Web3j web3j,
                Credentials credentials,
                GasProvider contractGasProvider,
                Long chainId) {
            super(contractAddress, web3j, credentials, contractGasProvider, chainId);
        }
    }

    @BeforeAll
    public static void init() throws Exception {
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
        simpleContract =
                Simple_contract.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
    }

    @Test
    @Order(1)
    public void testGetOwners() throws Exception {
        System.out.println("testGetOwners");
        WasmAddress[] resultOwners = multisig.GetOwners().send();
        Assertions.assertEquals(resultOwners[0], owners[0]);
        Assertions.assertEquals(resultOwners[1], owners[1]);
        Assertions.assertEquals(resultOwners[2], owners[2]);
    }

    @Test
    @Order(2)
    public void testGetRequired() throws Exception {
        System.out.println("testGetRequired");
        Assertions.assertEquals(Uint16.of(2), multisig.GetRequired().send());
    }

    @Test
    @Order(3)
    public void testPushTransaction() throws Exception {
        System.out.println("testPushTransaction");
        byte[] paras = {
            (byte) 0xca,
            (byte) 0x88,
            (byte) 0xd8,
            (byte) 0x9f,
            (byte) 0x9d,
            (byte) 0x18,
            (byte) 0x6b,
            (byte) 0x7b,
            (byte) 0xb3,
            (byte) 0x67,
            (byte) 0x07
        };
        WasmAddress to = new WasmAddress(simpleContract.getContractAddress());
        TransactionReceipt receipt = multisig.PushTransaction(to, paras, Uint32.of(200000)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        Multisig.Transaction[] transactions = multisig.GetPendingTransactions().send();
        System.out.println(transactions[0].to_);

        Multisig.Transaction one = multisig.GetTransactionInfo(Uint32.of(1)).send();
        System.out.println(one.to_);
        System.out.println(one.sender_);

        Assertions.assertEquals(transactions[0].to_, one.to_);

        MultisigInner signMultisig =
                new MultisigInner(
                        multisig.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user1,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        TransactionReceipt signReceipt = signMultisig.SignTransaction(Uint32.of(1)).send();
        Assertions.assertTrue(signReceipt.isStatusOK());

        transactions = multisig.GetPendingTransactions().send();
        Assertions.assertTrue(0 == transactions.length);

        Uint64 setValue = simpleContract.get().send();
        System.out.println(setValue);
        Assertions.assertEquals(setValue, Uint64.of(7));
    }

    @Test
    @Order(4)
    public void testChangeOwnersAndRequired() throws Exception {
        System.out.println("testChangeOwnersAndRequired");
        owners = new WasmAddress[2];
        owners[0] = new WasmAddress(Web.admin.getAddress());
        owners[1] = new WasmAddress(Web.user1.getAddress());
        TransactionReceipt receipt =
                multisig.ChangeOwnersAndRequired(owners, Uint16.of(1), Uint32.of(200000)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        Multisig.Transaction[] transactions = multisig.GetPendingTransactions().send();
        Assertions.assertTrue(1 == transactions.length);
        System.out.println(transactions[0].to_);

        Multisig.Transaction one = multisig.GetTransactionInfo(Uint32.of(1)).send();
        System.out.println(one.to_);
        System.out.println(one.sender_);

        Assertions.assertEquals(transactions[0].to_, one.to_);

        MultisigInner signMultisig =
                new MultisigInner(
                        multisig.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user1,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        TransactionReceipt signReceipt = signMultisig.SignTransaction(Uint32.of(1)).send();
        Assertions.assertTrue(signReceipt.isStatusOK());

        WasmAddress[] resultOwners = multisig.GetOwners().send();
        Assertions.assertTrue(2 == resultOwners.length);
        Assertions.assertEquals(resultOwners[0], owners[0]);
        Assertions.assertEquals(resultOwners[1], owners[1]);

        Assertions.assertEquals(Uint16.of(1), multisig.GetRequired().send());

        simpleContract.set(Uint64.of(1)).send();
        Uint64 setValue = simpleContract.get().send();
        System.out.println(setValue);
        Assertions.assertEquals(setValue, Uint64.of(1));

        byte[] paras = {
            (byte) 0xca,
            (byte) 0x88,
            (byte) 0xd8,
            (byte) 0x9f,
            (byte) 0x9d,
            (byte) 0x18,
            (byte) 0x6b,
            (byte) 0x7b,
            (byte) 0xb3,
            (byte) 0x67,
            (byte) 0x07
        };
        WasmAddress to = new WasmAddress(simpleContract.getContractAddress());
        TransactionReceipt pushReceipt =
                multisig.PushTransaction(to, paras, Uint32.of(200000)).send();
        Assertions.assertTrue(pushReceipt.isStatusOK());

        setValue = simpleContract.get().send();
        System.out.println(setValue);
        Assertions.assertEquals(setValue, Uint64.of(7));
    }
}
