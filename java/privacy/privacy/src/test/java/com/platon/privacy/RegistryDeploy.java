package com.platon.privacy;

import com.alaya.abi.wasm.WasmFunctionEncoder;
import com.alaya.bech32.Bech32;
import com.alaya.crypto.*;
import com.alaya.protocol.core.methods.response.PlatonSendTransaction;
import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.solidity.RlpEncoder;
import com.alaya.rlp.solidity.RlpList;
import com.alaya.rlp.solidity.RlpString;
import com.alaya.rlp.solidity.RlpType;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.alaya.tx.Transfer;
import com.alaya.tx.response.PollingTransactionReceiptProcessor;
import com.alaya.utils.Bytes;
import com.alaya.utils.Convert;
import com.alaya.utils.Numeric;
import com.platon.privacy.contracts.Registry;

import org.apache.commons.codec.binary.Hex;


import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static com.alaya.crypto.TransactionEncoder.createEip155SignatureData;
import static com.alaya.utils.Numeric.hexStringToByteArray;

public class RegistryDeploy {
    public static WasmAddress registryAddress;
    public static WasmAddress oneTimeAccount;

    public static byte[] encodeRawTransaction(
            RawTransaction rawTransaction, Sign.SignatureData signatureData) {
        List<RlpType> result = new ArrayList<>();

        result.add(RlpString.create(rawTransaction.getNonce()));
        result.add(RlpString.create(rawTransaction.getGasPrice()));
        result.add(RlpString.create(rawTransaction.getGasLimit()));

        // an empty to address (contract creation) should not be encoded as a numeric 0 value
        String to = rawTransaction.getTo();
        if (to != null && to.length() > 0) {
            // addresses that start with zeros should be encoded with the zeros included, not
            // as numeric values
            result.add(RlpString.create(Bech32.addressDecode(to)));
        } else {
            result.add(RlpString.create(""));
        }

        result.add(RlpString.create(rawTransaction.getValue()));

        // value field will already be hex encoded, so we need to convert into binary first
        byte[] data = hexStringToByteArray(rawTransaction.getData());
        result.add(RlpString.create(data));

        if (signatureData != null) {
            result.add(RlpString.create(Bytes.trimLeadingZeroes(signatureData.getV())));
            result.add(RlpString.create(Bytes.trimLeadingZeroes(signatureData.getR())));
            result.add(RlpString.create(Bytes.trimLeadingZeroes(signatureData.getS())));
        }

        RlpList rlpList = new RlpList(result);
        return RlpEncoder.encode(rlpList);
    }

    public static void deployRegistryOneTimeAccount(String tokenType) throws Exception {
        BigInteger gasPrice = Web.chainManager.getGasProvider().getGasPrice();
        BigInteger gasLimit = Web.chainManager.getGasProvider().getGasLimit();
        BigInteger value = BigInteger.ZERO;
        BigInteger nonce = BigInteger.ZERO;
        String data = WasmFunctionEncoder.encodeConstructor(Registry.BINARY, Arrays.asList());
        RawTransaction rawTransaction =
                RawTransaction.createTransaction(nonce, gasPrice, gasLimit, null, value, data);
        // r,s,v
        String groupId = "chain";
        byte[] r = Hash.sha3(groupId.getBytes(StandardCharsets.UTF_8));

        String contractName = "registry" + tokenType;
        byte[] s = Hash.sha3(contractName.getBytes(StandardCharsets.UTF_8));
        BigInteger origin = new BigInteger(s);
        BigInteger secp256k1N =
                new BigInteger(
                        "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141", 16);
        BigInteger secp256k1halfN = secp256k1N.divide(new BigInteger("2"));
        s = origin.add(secp256k1halfN).toByteArray();

        byte[] v = new byte[] {(byte) 27};

        Sign.SignatureData signatureData = new Sign.SignatureData(v, r, s);
        Sign.SignatureData eip155SignatureData =
                createEip155SignatureData(signatureData, Web.chainManager.getChainId());
        byte[] signedMessage = encodeRawTransaction(rawTransaction, eip155SignatureData);

        String hexValue = Numeric.toHexString(signedMessage);

        SignedRawTransaction transaction =
                (SignedRawTransaction) TransactionDecoder.decode(hexValue);
        String addr = transaction.getFrom();
        System.out.println(addr);
        oneTimeAccount = new WasmAddress(addr);
        System.out.println(Hex.encodeHexString(oneTimeAccount.getValue()));
        System.out.println(hexValue);

        TransactionReceipt receipt =
                Transfer.sendFunds(
                                Web.chainManager.getWeb3j(),
                                Web.admin,
                                Web.chainManager.getChainId(),
                                oneTimeAccount.getAddress(),
                                new BigDecimal("10"),
                                Convert.Unit.ATP)
                        .send();
        System.out.println(receipt.getStatus());

        PlatonSendTransaction ethSendTransaction =
                Web.chainManager.getWeb3j().platonSendRawTransaction(hexValue).send();
        if (ethSendTransaction.hasError()) {
            throw new RuntimeException(
                    "Error processing transaction request: "
                            + ethSendTransaction.getError().getMessage());
        }

        String transactionHash = ethSendTransaction.getTransactionHash();

        PollingTransactionReceiptProcessor receiptProcess =
                new PollingTransactionReceiptProcessor(Web.chainManager.getWeb3j(), 2 * 1000, 40);
        receipt = receiptProcess.waitForTransactionReceipt(transactionHash);
        System.out.println(receipt.getStatus());

        registryAddress = new WasmAddress(receipt.getContractAddress());
        System.out.println("registry contract address:" + registryAddress.getAddress());
    }

    public static void deployRegistry(String tokenType) throws Exception {
        Registry registry = Registry.deploy(Web.chainManager.getWeb3j(), Web.admin, Web.chainManager.getGasProvider(), Web.chainManager.getChainId()).send();
        registryAddress = new WasmAddress(registry.getContractAddress());
    }
}
