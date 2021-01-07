[中文](./README_cn.md)

# confidential token

Confidential token is a privacy token project based on Alaya's WASM smart contract platform PIP-13 proposal. The project is intended to address the increasing demand for anonymization of transactions. The zero-knowledge proof algorithm is used to achieve hidden identity, and the algorithm is plug-in to meet the needs of multiple privacy; it supports the independent issuance of privacy tokens and the privacy of ARC20 assets; the plug-in mechanism is used to achieve online upgrades of the algorithm to ensure that users No sense of upgrade.

## Compile

Install the WASM smart contract platform development kit `PlatON-CDT`, refer to [PlatON-CDT README](https://github.com/PlatONnetwork/PlatON-CDT/blob/feature/wasm/README_cn.md).

Executing `./contracts/build.sh` will place the compiled wam files and abi files of each contract in the build directory.

## Contract deployment process

1. Registry The registry contract is used to manage the ACL address. After each ACL upgrade, the ACL contract address registered in the Registry will be updated synchronously.

2. (Optional) MultiSign multi-signature contracts are used to manage ACL contract deployment and upgrades, and register new versions of Validator and Storage contracts.

3. ARC20 token contract, maintain the ARC20 token of each address and provide corresponding token operation methods.

4. TokenManager template contract. The TokenManager contract interacts with the ARC20 contract and transfers tokens in and out to the TokenManager contract.

5. ACL controls the management contract. During the deployment process, the TokenManager contract will be cloned to realize the interaction with the ARC20 token contract.

6. The Validator verifies the contract and needs to be registered with ACL after deployment.

7. Storage storage contract needs to be registered with ACL after deployment.

8. ConfidentialToken privacy token contract, the deployment depends on the Registry contract address, the Validator contract version and the Storage contract version registered in ACL.

Code example, specific writing can refer to unit test:

```java
    Token token = new Token();

    // deploy Registry
    Random rand = new Random();
    int num = rand.nextInt(100);
    confidentialDeploy += num;
    RegistryDeploy.deployRegistry(confidentialDeploy);

    // deploy ARC20
    token.arc20 =  Arc20.deploy( Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId(),
                            tokenName,
                            tokenSymbol,
                            totalSupply,
                            decimals).send();

    // deploy TokenManager
    token.tokenManager = Token_manager.deploy(
                            Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId(),
                            new WasmAddress(BigInteger.ZERO)).send();

    // deploy acl
    token.acl = Acl.deploy(
                        Web.chainManager.getWeb3j(),
                        Web.txManager,
                        Web.chainManager.getGasProvider(),
                        Web.chainManager.getChainId(),
                        RegistryDeploy.registryAddress,
                        new WasmAddress(token.tokenManager.getContractAddress())).send();

    // deploy Validator
    token.validator = Confidential_validator.deploy(
                            Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId()).send();
    token.acl.CreateValidator(
                    Uint32.of(Confidential.currentVersion),
                    new WasmAddress(token.validator.getContractAddress()),
                    "").send();

    // deploy Storage
    token.storage = Confidential_storage.deploy(
                            Web.chainManager.getWeb3j(),
                            Web.txManager,
                            Web.chainManager.getGasProvider(),
                            Web.chainManager.getChainId()).send();
    token.acl.CreateStorage(
                    Uint32.of(Confidential.currentVersion),
                    new WasmAddress(token.storage.getContractAddress()),
                    "").send();

    // deploy ConfidentialToken
    token.confidentialToken = Confidential_token.deploy(
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
                            new WasmAddress(token.arc20.getContractAddress())).send();
```

## unit test

When the unit test is implemented with java, the java environment needs to be installed.

In the unit test part, the default is to install the underlying chain of docker image for unit testing. It also supports configuring your own environment. You need to change the `Web.selfTestNode = true;`, and configure the `conf.yml` file of the underlying chain.

## License

GNU General Public License v3.0, see [LICENSE](https://github.com/PlatONnetwork/PlatON-CDT/blob/master/LICENSE).
