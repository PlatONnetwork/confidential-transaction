package com.platon.privacy;

import com.alaya.rlp.wasm.datatypes.Uint128;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.Uint8;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.platon.privacy.contracts.*;
import com.platon.privacy.plaintext.Plaintext;
import org.apache.commons.codec.binary.Hex;

import java.math.BigInteger;

public class Deploy {
    public static String plaintextDeploy = "plaintext";
    public static String tokenName = "ABC";
    public static String tokenSymbol = "abc";
    public static Uint128 scalingFactor = Uint128.of(10);
    public static Uint128 totalSupply = Uint128.of(100000);
    public static Uint8 decimals = Uint8.of(10);

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
                Storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.validator =
                Plaintext_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        int num = 100;
        plaintextDeploy += num;
        RegistryDeploy.deployRegistry(plaintextDeploy);
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
                        Uint32.of(Plaintext.currentVersion),
                        new WasmAddress(token.validator.getContractAddress()),
                        "")
                .send();
        token.acl.CreateStorage(
                        Uint32.of(Plaintext.currentVersion),
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
                                Uint32.of(Plaintext.currentVersion),
                                Uint32.of(Plaintext.currentVersion),
                                scalingFactor,
                                new WasmAddress(token.arc20.getContractAddress()))
                        .send();
        token.arc20.Approve(new WasmAddress(token.acl.getContractAddress()), Uint128.of(1000))
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

    public static Storage deployStorage() throws Exception {
        return Storage.deploy(
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId())
                .send();
    }

    public static Plaintext_validator deployPlaintextValidator() throws Exception {
        return Plaintext_validator.deploy(
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
                Storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.validator =
                Plaintext_validator.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
        token.acl = deployAcl(needRegister, deployType, tokenManager);
        token.acl.CreateValidator(
                        Uint32.of(Plaintext.currentVersion),
                        new WasmAddress(token.validator.getContractAddress()),
                        "")
                .send();
        token.acl.CreateStorage(
                        Uint32.of(Plaintext.currentVersion),
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
            arc20Addr = new WasmAddress(token.arc20.getContractAddress());
        }

        token.acl.CreateRegistry(
                        Uint32.of(Plaintext.currentVersion),
                        Uint32.of(Plaintext.currentVersion),
                        scalingFactor,
                        arc20Addr,
                        mintBurn)
                .send();
        return token;
    }

    public static class Token {
        public Arc20 arc20;
        public Storage storage;
        public Plaintext_validator validator;
        public Acl acl;
        public Confidential_token confidentialToken;
        public Token_manager tokenManager;
    }
}
