package com.platon.privacy;

import com.alaya.crypto.Credentials;
import com.alaya.tx.RawTransactionManager;
import com.platon.privacy.chain.ChainManager;
import org.testcontainers.containers.GenericContainer;
import org.testcontainers.containers.wait.strategy.Wait;
import org.testcontainers.utility.DockerImageName;

import org.yaml.snakeyaml.Yaml;

import java.io.InputStream;
import java.math.BigInteger;
import java.util.Map;

public class Web {
    public static final GenericContainer singleNode =
            new GenericContainer(
                            DockerImageName.parse(
                                    "registry.cn-hangzhou.aliyuncs.com/pressure/docker:single-platon"))
                    .withExposedPorts(6601);
    public static boolean selfTestNode = false;
    public static String confFileName = "conf.yml";
    public static ChainManager chainManager;
    public static Credentials admin;
    public static RawTransactionManager txManager;
    public static Credentials user1;
    public static RawTransactionManager user1Manager;
    public static Credentials user2;
    public static RawTransactionManager user2Manager;
    public static String chainUrl;

    static {
        init();
    }

    public static void init() {
        if (!selfTestNode) {
            singleNode.waitingFor(Wait.forListeningPort());
            singleNode.start();
            chainUrl = "ws://127.0.0.1:" + singleNode.getMappedPort(6601);
        }
        Yaml yaml = new Yaml();
        InputStream inputStream = Web.class.getClassLoader().getResourceAsStream(confFileName);
        Map<String, Object> obj = yaml.load(inputStream);

        if (selfTestNode) {
            chainUrl = obj.get("url").toString();
        }

        long chainId = ((Number) obj.get("chainId")).longValue();
        BigInteger gasPrice = BigInteger.valueOf(((Number) obj.get("gasPrice")).longValue());
        BigInteger gasLimit = BigInteger.valueOf(((Number) obj.get("gasLimit")).longValue());
        chainManager = new ChainManager(chainUrl, chainId, gasPrice, gasLimit);

        admin = Credentials.create(obj.get("admin").toString());
        txManager = chainManager.createRawTransactionManager(admin);

        user1 = Credentials.create(obj.get("user1").toString());
        user1Manager = new RawTransactionManager(chainManager.getWeb3j(), user1, chainId);

        user2 = Credentials.create(obj.get("user2").toString());
        user2Manager = new RawTransactionManager(chainManager.getWeb3j(), user2, chainId);
    }
}
