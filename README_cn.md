
# confidential token

confidential token 是基于 Alaya 的 WASM 智能合约平台 PIP-13 提案构建的隐私代币项目。该项目意在解决日益剧增的交易匿名化需求。通过零知识证明算法达到隐匿身份，算法插件化来满足多种隐私的需求；支持隐私代币的独立发行以及 ARC20 资产的隐私化；利用插件机制，做到算法的在线升级，做到对用户的无感升级。

## 编译

安装 WASM 智能合约平台开发套件 `PlatON-CDT`, 参考 [PlatON-CDT README](https://github.com/PlatONnetwork/PlatON-CDT/blob/feature/wasm/README_cn.md)。

执行 `./contracts/build.sh`，会将每个合约编译好的 wam 文件 和 abi 文件 放置到 build 目录下。

## 合约部署流程

1. Registry 注册表合约，用来管理 ACL 地址，每次 ACL 升级后会同步更新 Registry 中注册的 ACL 合约地址。

2. （非必选）MultiSign 多签合约用来管理 ACL 合约部署升级，注册新版本的 Validator 和 Storage 合约。

3. ARC20 代币合约，维护各地址的 ARC20 token 和提供相应的 token 操作方法。

4. TokenManager 模板合约，TokenManager 合约与ARC20 合约交互，转入转出 token 到 TokenManager 合约。

5. ACL 控制管理合约，部署过程中会 clone 出 TokenManager 合约， 实现和 ARC20 代币合约进行交互。

6. Validator 验证合约，部署后需要在 ACL 进行注册。

7. Storage 存储合约， 部署后需要在 ACL 进行注册。

8. ConfidentialToken 隐私代币合约， 部署依赖 Registry 合约地址， ACL 里面注册的 Validator 合约版本和 Storage 合约版本。

代码例子，具体写法可以参考单元测试：

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

## 单元测试

单元测试时用 java 实现的，需要安装 java 环境。

单元测试部分默认是 docker 镜像安装底层链进行单元测试，也支持配置自己的环境， 需要更改 `Web.selfTestNode = true;`, 并配置底层链的 `conf.yml` 文件。

## License

GNU General Public License v3.0, see [LICENSE](https://github.com/PlatONnetwork/PlatON-CDT/blob/master/LICENSE).