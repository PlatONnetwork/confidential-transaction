package com.platon.privacy;

import com.alaya.crypto.*;
import com.alaya.protocol.core.DefaultBlockParameterName;
import com.alaya.protocol.core.methods.response.*;
import com.alaya.tx.Transfer;
import com.alaya.tx.response.PollingTransactionReceiptProcessor;
import com.alaya.utils.Convert;
import com.platon.privacy.contracts.Arc20;
import com.alaya.rlp.wasm.datatypes.*;
import com.platon.privacy.confidential.Confidential;
import com.platon.privacy.contracts.*;
import com.alaya.abi.wasm.WasmEventValues;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.tx.WasmContract;
import com.alaya.utils.Numeric;
import org.junit.Assert;
import org.junit.BeforeClass;
import org.junit.Test;
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

import java.math.BigDecimal;
import java.math.BigInteger;

public class ARC20Test {
    private static Arc20 Arc20;
    private static final String tokenName = "ABC";
    private static final String tokenSymbol = "abc";
    private static final Uint128 totalSupply = Uint128.of(100000);
    private static final Uint8 decimals = Uint8.of(10);

    @BeforeClass
    public static void deployContract() throws Exception {
        PollingTransactionReceiptProcessor txp =
                new PollingTransactionReceiptProcessor(Web.chainManager.getWeb3j(), 1000, 10);

        Arc20 =
                com.platon.privacy.contracts.Arc20.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                tokenName,
                                tokenSymbol,
                                totalSupply,
                                decimals)
                        .send();
        PlatonGetTransactionCount count =
                Web.chainManager
                        .getWeb3j()
                        .platonGetTransactionCount(
                                Web.admin.getAddress(), DefaultBlockParameterName.PENDING)
                        .send();
        if (Web.chainManager
                        .getWeb3j()
                        .platonGetBalance(Web.user1.getAddress(), DefaultBlockParameterName.PENDING)
                        .send()
                        .getBalance()
                        .compareTo(new BigInteger("90010000000000000"))
                < 1) {
            RawTransaction tx =
                    RawTransaction.createEtherTransaction(
                            count.getTransactionCount(),
                            Web.chainManager.getGasProvider().getGasPrice(),
                            Web.chainManager.getGasProvider().getGasLimit(),
                            Web.user1.getAddress(),
                            BigInteger.valueOf(900000000).multiply(BigInteger.valueOf(100000000)));
            txp.waitForTransactionReceipt(Web.txManager.signAndSend(tx).getTransactionHash());
            System.out.println(
                    "balance:"
                            + Web.chainManager
                                    .getWeb3j()
                                    .platonGetBalance(
                                            Web.user1.getAddress(),
                                            DefaultBlockParameterName.PENDING)
                                    .send()
                                    .getBalance());
        }
        if (Web.chainManager
                        .getWeb3j()
                        .platonGetBalance(Web.user2.getAddress(), DefaultBlockParameterName.PENDING)
                        .send()
                        .getBalance()
                        .compareTo(new BigInteger("90010000000000000"))
                < 1) {

            Transfer.sendFunds(
                            Web.chainManager.getWeb3j(),
                            Web.admin,
                            Web.chainManager.getChainId(),
                            Web.user2.getAddress(),
                            new BigDecimal(900000000).multiply(BigDecimal.valueOf(100000000)),
                            Convert.Unit.VON)
                    .send();
            System.out.println(
                    "balance:"
                            + Web.chainManager
                                    .getWeb3j()
                                    .platonGetBalance(
                                            Web.user2.getAddress(),
                                            DefaultBlockParameterName.PENDING)
                                    .send()
                                    .getBalance());
        }
    }

    @Test
    public void testGetTokenName() throws Exception {
        Assert.assertEquals(tokenSymbol, Arc20.GetSymbol().send());
        Assert.assertEquals(tokenName, Arc20.GetName().send());
        Assert.assertEquals(totalSupply, Arc20.GetTotalSupply().send());
        Assert.assertEquals(decimals, Arc20.GetDecimals().send());
    }

    @Test
    public void testApprove() throws Exception {
        Uint128 value = Uint128.of(10000);
        TransactionReceipt receipt =
                Arc20.Approve(new WasmAddress(Web.user1.getAddress()), value).send();
        Assert.assertTrue(receipt.isStatusOK());

        Uint128 allowance =
                Arc20.Allowance(
                                new WasmAddress(Web.admin.getAddress()),
                                new WasmAddress(Web.user1.getAddress()))
                        .send();
        Assert.assertEquals(value, allowance);

        receipt =
                Arc20.IncreaseApprove(
                                new WasmAddress(Web.user1.getAddress()),
                                Uint128.of(value.getValue().add(value.value)))
                        .send();
        Assert.assertTrue(receipt.isStatusOK());

        receipt = Arc20.DecreaseApprove(new WasmAddress(Web.user1.getAddress()), value).send();
        Assert.assertTrue(receipt.isStatusOK());

        allowance =
                Arc20.Allowance(
                                new WasmAddress(Web.admin.getAddress()),
                                new WasmAddress(Web.user1.getAddress()))
                        .send();
        Assert.assertEquals(Uint128.of(value.getValue().add(value.value)), allowance);

        Arc20 ue =
                com.platon.privacy.contracts.Arc20.load(
                        Arc20.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user1Manager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        ue.TransferFrom(
                        new WasmAddress(Web.admin.getAddress()),
                        new WasmAddress(BigInteger.ONE),
                        Uint128.of(1000))
                .send();
        Assert.assertEquals(
                BigInteger.valueOf(1000),
                ue.BalanceOf(new WasmAddress(BigInteger.ONE)).send().getValue());
    }

    @Test
    public void testTransfer() throws Exception {
        WasmAddress tmpAddr =
                new WasmAddress(
                        new byte[] {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
        TransactionReceipt receipt = Arc20.Transfer(tmpAddr, Uint128.of(10)).send();
        Assert.assertTrue(receipt.isStatusOK());
        Assert.assertEquals(Arc20.BalanceOf(tmpAddr).send().getValue(), BigInteger.valueOf(10));
    }

    @Test
    public void testMint() throws Exception {
        WasmAddress tmpAddr =
                new WasmAddress(
                        new byte[] {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
        Arc20.Mint(tmpAddr, Uint128.of(100)).send();
        Assert.assertEquals(Arc20.BalanceOf(tmpAddr).send().getValue(), BigInteger.valueOf(100));
    }

    @Test
    public void testBurn() throws Exception {
        Uint128 balance = Arc20.BalanceOf(new WasmAddress(Web.admin.getAddress())).send();
        TransactionReceipt receipt =
                Arc20.Burn(new WasmAddress(Web.admin.getAddress()), Uint128.of(100)).send();
        Assert.assertTrue(receipt.isStatusOK());
        Uint128 remain = Arc20.BalanceOf(new WasmAddress(Web.admin.getAddress())).send();
        Assert.assertEquals(balance.getValue(), remain.getValue().add(BigInteger.valueOf(100)));
    }

    @Test
    public void testTransferOverflow() throws Exception {
        // illegal Arc20 sender
        Arc20 contract =
                com.platon.privacy.contracts.Arc20.load(
                        Arc20.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user2Manager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());

        Boolean status = true;
        try {
            contract.Transfer(new WasmAddress(Web.admin.getAddress()), Uint128.of(1000)).send();
        } catch (Exception e) {
            status = false;
        }

        Assert.assertFalse(status);

        TransactionReceipt receipt =
                Arc20.Transfer(new WasmAddress(Web.user2.getAddress()), Uint128.of(1000)).send();
        Assert.assertTrue(receipt.isStatusOK());
        Assert.assertEquals(
                BigInteger.valueOf(1000),
                contract.BalanceOf(new WasmAddress(Web.user2.getAddress())).send().getValue());

        receipt = contract.Transfer(new WasmAddress(Web.admin.getAddress()), Uint128.of(10)).send();
        Assert.assertTrue(receipt.isStatusOK());
        Assert.assertEquals(
                BigInteger.valueOf(990),
                contract.BalanceOf(new WasmAddress(Web.user2.getAddress())).send().getValue());

        status = true;
        try {
            contract.Transfer(new WasmAddress(Web.admin.getAddress()), Uint128.of(100000)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        receipt =
                contract.Transfer(new WasmAddress(Web.admin.getAddress()), Uint128.of(990)).send();
        Assert.assertTrue(receipt.isStatusOK());

        Assert.assertEquals(
                BigInteger.ZERO,
                contract.BalanceOf(new WasmAddress(Web.user2.getAddress())).send().getValue());
    }

    @Test
    public void testIllegalApprove() throws Exception {
        WasmAddress tmpAddr = new WasmAddress(BigInteger.valueOf(7));

        Boolean status = true;
        try {
            Arc20.Approve(tmpAddr, Uint128.of(0)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        TransactionReceipt receipt = Arc20.Approve(tmpAddr, Uint128.of(100)).send();
        Assert.assertTrue(receipt.isStatusOK());
        Assert.assertEquals(1, receipt.getLogs().size());
        Assert.assertEquals(
                BigInteger.valueOf(100),
                Arc20.getApprovalEventEvents(receipt).get(0).arg1.getValue());

        status = true;
        try {
            Arc20.IncreaseApprove(tmpAddr, Uint128.of(0)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        status = true;
        try {
            Arc20.DecreaseApprove(tmpAddr, Uint128.of(1000)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        Assert.assertEquals(
                BigInteger.valueOf(100),
                Arc20.Allowance(new WasmAddress(Web.admin.getAddress()), tmpAddr)
                        .send()
                        .getValue());
    }

    @Test
    public void testIllegalMint() throws Exception {
        Arc20 contract =
                com.platon.privacy.contracts.Arc20.load(
                        Arc20.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user2Manager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        Boolean status = true;
        try {
            contract.Mint(new WasmAddress(Web.user2.getAddress()), Uint128.of(1)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        status = true;
        try {
            Arc20.Mint(new WasmAddress(BigInteger.ZERO), Uint128.of(1)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        status = true;
        try {
            Arc20.Mint(new WasmAddress(BigInteger.ONE), Uint128.of(0)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);
    }

    @Test
    public void testIllegalBurn() throws Exception {
        Arc20 contract =
                com.platon.privacy.contracts.Arc20.load(
                        Arc20.getContractAddress(),
                        Web.chainManager.getWeb3j(),
                        Web.user2Manager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId());
        Boolean status = true;
        try {
            contract.Burn(new WasmAddress(Web.user2.getAddress()), Uint128.of(1)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        WasmAddress tmpAddr =
                new WasmAddress(
                        new byte[] {8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
        Arc20.Transfer(tmpAddr, Uint128.of(10)).send();
        Assert.assertEquals(BigInteger.valueOf(10), Arc20.BalanceOf(tmpAddr).send().getValue());

        status = true;
        try {
            Arc20.Burn(tmpAddr, Uint128.of(100)).send();
        } catch (Exception e) {
            status = false;
        }
        Assert.assertFalse(status);

        Arc20.Burn(tmpAddr, Uint128.of(10)).send();
        Assert.assertEquals(BigInteger.ZERO, Arc20.BalanceOf(tmpAddr).send().getValue());
    }
}
