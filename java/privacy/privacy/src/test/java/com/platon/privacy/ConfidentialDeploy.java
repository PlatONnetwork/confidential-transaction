package com.platon.privacy;

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
import org.apache.commons.codec.binary.Hex;
import org.bouncycastle.util.Arrays;


import java.math.BigInteger;
import java.util.Random;

public class ConfidentialDeploy {
    public static String confidentialDeploy = "confidential";
    public static String tokenName = "ABC";
    public static String tokenSymbol = "abc";
    public static Uint128 scalingFactor = Uint128.of(10);
    public static Uint128 totalSupply = Uint128.of(100000);
    public static Uint8 decimals = Uint8.of(10);

    public static String encodeEventName(String name) {
        byte[] data = name.getBytes(Charsets.UTF_8);
        return Numeric.toHexString(Arrays.concatenate(new byte[32 - data.length], data));
    }

    public static Token deploy() throws Exception {
        Token token = new Token();
        token.arc20 =
                Arc20.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                tokenName,
                                tokenSymbol,
                                totalSupply,
                                decimals)
                        .send();
        token.tokenManager =
                Token_manager.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                new WasmAddress(BigInteger.ZERO))
                        .send();
        token.storage =
                Confidential_storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.validator =
                Confidential_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        Random rand = new Random();
        int num = rand.nextInt(100);
        confidentialDeploy += num;
        RegistryDeploy.deployRegistry(confidentialDeploy);
        token.acl =
                Acl.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                RegistryDeploy.registryAddress,
                                new WasmAddress(token.tokenManager.getContractAddress()))
                        .send();
        token.acl.CreateValidator(
                        Uint32.of(Confidential.currentVersion),
                        new WasmAddress(token.validator.getContractAddress()),
                        "")
                .send();
        token.acl.CreateStorage(
                        Uint32.of(Confidential.currentVersion),
                        new WasmAddress(token.storage.getContractAddress()),
                        "")
                .send();

        token.confidentialToken =
                Confidential_token.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId(),
                                tokenName,
                                tokenSymbol,
                                RegistryDeploy.registryAddress,
                                Uint32.of(Confidential.currentVersion),
                                Uint32.of(Confidential.currentVersion),
                                scalingFactor,
                                new WasmAddress(token.arc20.getContractAddress()))
                        .send();

        System.out.println(
                "admin:"
                        + Web.admin.getAddress()
                        + "  "
                        + Hex.encodeHexString(
                                Web.admin.getEcKeyPair().getPrivateKey().toByteArray()));

        System.out.println("arc20:" + token.arc20.getContractAddress());
        System.out.println("storage:" + token.storage.getContractAddress());
        System.out.println("validator:" + token.validator.getContractAddress());
        System.out.println("acl:" + token.acl.getContractAddress());
        System.out.println(
                "Confidential_token:"
                        + token.confidentialToken.getContractAddress()
                        + " name:"
                        + token.confidentialToken.Name().send()
                        + " symbol:"
                        + token.confidentialToken.Symbol().send());
        return token;
    }

    public static void minorUpdate(Token token) throws Exception {
        Confidential_storage minorUpdateStorage =
                Confidential_storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        Confidential_validator minorUpdateValidator =
                Confidential_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();

        token.acl.CreateStorage(
                        Uint32.of(Confidential.minorUpdateVersion),
                        new WasmAddress(minorUpdateStorage.getContractAddress()),
                        "")
                .send();
        token.acl.CreateValidator(
                        Uint32.of(Confidential.minorUpdateVersion),
                        new WasmAddress(minorUpdateValidator.getContractAddress()),
                        "")
                .send();

        token.confidentialToken.UpdateStorage(Uint32.of(Confidential.minorUpdateVersion)).send();
        token.confidentialToken.UpdateValidator(Uint32.of(Confidential.minorUpdateVersion)).send();
    }

    public static void majorUpdate(Token token) throws Exception {
        Confidential_storage majorUpdateStorage =
                Confidential_storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        Confidential_validator majorUpdateValidator =
                Confidential_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();

        token.acl.CreateStorage(
                        Uint32.of(Confidential.majorUpdateVersion),
                        new WasmAddress(majorUpdateStorage.getContractAddress()),
                        "")
                .send();
        token.acl.CreateValidator(
                        Uint32.of(Confidential.majorUpdateVersion),
                        new WasmAddress(majorUpdateValidator.getContractAddress()),
                        "")
                .send();

        token.confidentialToken.UpdateStorage(Uint32.of(Confidential.majorUpdateVersion)).send();
        token.confidentialToken.UpdateValidator(Uint32.of(Confidential.majorUpdateVersion)).send();
    }

    public static void updateAcl(Token token) throws Exception {
        String tokenManagerAddress = token.tokenManager.getContractAddress();
        token.tokenManager.setContractAddress(new WasmAddress(BigInteger.ZERO).getAddress());
        Acl newAcl = deployAcl(false, confidentialDeploy, token.tokenManager);
        token.tokenManager.setContractAddress(tokenManagerAddress);
        TransactionReceipt receipt =
                token.acl.Migrate(new WasmAddress(newAcl.getContractAddress())).send();
        String migrateEvent = encodeEventName(Acl.ACLMIGRATEEVENT_EVENT.getName());
        for (Log log : receipt.getLogs()) {
            String name = log.getTopics().get(0);
            if (name.equals(migrateEvent)) {
                WasmEventValues values =
                        WasmContract.staticExtractEventParameters(
                                Acl.ACLMIGRATEEVENT_EVENT, log, Web.chainManager.getChainId());

                String newAddress = values.getIndexedValues().get(1);
                byte[] newAddressBytes = Hex.decodeHex(newAddress.substring(2));
                token.acl.setContractAddress(new WasmAddress(newAddressBytes).getAddress());
            }
        }
    }

    public static Arc20 deployArc20() throws Exception {
        return Arc20.deploy(
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId(),
                        tokenName,
                        tokenSymbol,
                        totalSupply,
                        decimals)
                .send();
    }

    public static Confidential_storage deployConfidentialStorage() throws Exception {
        return Confidential_storage.deploy(
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId())
                .send();
    }

    public static Confidential_validator deployConfidentialValidator() throws Exception {
        return Confidential_validator.deploy(
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId())
                .send();
    }

    public static Acl deployAcl(boolean needRegister, String deployType, Token_manager tokenManager)
            throws Exception {
        if (needRegister) {
            RegistryDeploy.deployRegistry(deployType);
            return Acl.deploy(
                            Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId(),
                            RegistryDeploy.registryAddress,
                            new WasmAddress(tokenManager.getContractAddress()))
                    .send();
        } else {
            return Acl.deploy(
                            Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId(),
                            new WasmAddress(BigInteger.ZERO),
                            new WasmAddress(tokenManager.getContractAddress()))
                    .send();
        }
    }

    public static Token deployAcl(
            boolean arc20,
            boolean mintBurn,
            boolean needRegister,
            String deployType,
            Token_manager tokenManager)
            throws Exception {
        Token token = new Token();
        token.storage =
                Confidential_storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.validator =
                Confidential_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.acl = deployAcl(needRegister, deployType, tokenManager);
        token.acl.CreateValidator(
                        Uint32.of(Confidential.currentVersion),
                        new WasmAddress(token.validator.getContractAddress()),
                        "")
                .send();
        token.acl.CreateStorage(
                        Uint32.of(Confidential.currentVersion),
                        new WasmAddress(token.storage.getContractAddress()),
                        "")
                .send();
        WasmAddress arc20Addr = new WasmAddress(BigInteger.ZERO);
        if (arc20) {
            token.arc20 =
                    Arc20.deploy(
                                    Web.chainManager.getWeb3j(),
                                    Web.txManager,
                                    Web.chainManager.getGasProvider(),
                                    Web.chainManager.getChainId(),
                                    tokenName,
                                    tokenSymbol,
                                    totalSupply,
                                    decimals)
                            .send();
            token.arc20.Approve(new WasmAddress(token.acl.getContractAddress()), Uint128.of(1000))
                    .send();
            arc20Addr = new WasmAddress(token.arc20.getContractAddress());
        }

        token.acl.CreateRegistry(
                        Uint32.of(Confidential.currentVersion),
                        Uint32.of(Confidential.currentVersion),
                        scalingFactor,
                        arc20Addr,
                        mintBurn)
                .send();
        return token;
    }

    public static class Token {
        public Arc20 arc20;
        public Confidential_storage storage;
        public Confidential_validator validator;
        public Acl acl;
        public Confidential_token confidentialToken;
        public Token_manager tokenManager;
    }
}
