package com.platon.privacy.chain;



import com.alaya.crypto.Credentials;
import com.alaya.parameters.NetworkParameters;
import com.alaya.protocol.Web3j;
import com.alaya.protocol.http.HttpService;
import com.alaya.protocol.websocket.WebSocketService;
import com.alaya.tx.RawTransactionManager;
import com.alaya.tx.gas.ContractGasProvider;
import com.alaya.tx.gas.GasProvider;

import java.math.BigInteger;

public class ChainManager {
    private Web3j web3j;
    private long chainId = 101;
    private final GasProvider gasProvider;

    public ChainManager(String url, long chainId, BigInteger gasPrice, BigInteger gasLimit) {
        if (url.startsWith("ws")) {
            WebSocketService service = new WebSocketService(url, false);
            try {
                service.connect();
            } catch (Exception e) {
                service = null;
            }
            this.web3j = Web3j.build(service);
        } else if (url.startsWith("http")) {
            this.web3j = Web3j.build(new HttpService(url));
        }
        this.chainId = chainId;
        this.gasProvider = new ContractGasProvider(gasPrice, gasLimit);
        NetworkParameters.setCurrentNetwork(chainId);
    }

    public RawTransactionManager createRawTransactionManager(Credentials admin) {
        return new RawTransactionManager(web3j, admin, chainId);
    }

    public Web3j getWeb3j() {
        return web3j;
    }

    public long getChainId() {
        return chainId;
    }

    public GasProvider getGasProvider() {
        return gasProvider;
    }
}
