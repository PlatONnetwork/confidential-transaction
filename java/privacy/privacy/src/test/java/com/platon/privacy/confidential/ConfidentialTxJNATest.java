package com.platon.privacy.confidential;

import com.platon.privacy.Web;
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
import org.apache.commons.codec.binary.Hex;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Order;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;
import java.nio.file.Path;
import java.nio.file.Paths;

import static com.platon.privacy.confidential.ConfidentialTxJNA.is_note_owner;

public class ConfidentialTxJNATest {
    private final Confidential confidential;
    private final String authorizedAddress;

    public ConfidentialTxJNATest() throws Exception {
        Path path = Paths.get(System.getProperty("java.library.path") + "/confidentialtx.dll");
        System.out.println(path.toString());
        confidential = new Confidential(Web.chainManager.getChainId());
        WasmAddress adminAddress = new WasmAddress(Web.admin.getAddress());
        authorizedAddress = Hex.encodeHexString(adminAddress.getValue());
    }

    @Test
    @Order(1)
    void testRlpCreateConfidentialTxMint() throws Exception {
        // test mint
        Confidential.Mint mint = new Confidential.Mint();
        mint.authorized_address = Hex.decodeHex(authorizedAddress);
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(1)));
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(3)));
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(5)));
        byte[] buf = RLPCodec.encode(mint);
        System.out.println(Hex.encodeHexString(buf));
        byte[] txData = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);

        Confidential.ConfidentialUTXO utxo =
                RLPCodec.decode(
                        txData, Confidential.ConfidentialUTXO.class, Web.chainManager.getChainId());
        Assertions.assertEquals(utxo.publicValue, Uint64.of(9));
        WasmAddress adminAddress = new WasmAddress(Web.admin.getAddress());
        Assertions.assertEquals(utxo.authorizedAddress, adminAddress);
        Assertions.assertEquals(utxo.inputs.size(), 0);
        Assertions.assertEquals(utxo.outputs.size(), 3);

        int index = 0;
        for (Confidential.ConfidentialOutputNote output : utxo.outputs) {
            Assertions.assertTrue(
                    is_note_owner(
                            output.ephemeralPk,
                            output.ephemeralPk.length,
                            output.signPk,
                            output.signPk.length,
                            confidential.spendPk,
                            confidential.spendPk.length,
                            confidential.viewSk,
                            confidential.viewSk.length));

            byte[] plainBytes =
                    ConfidentialTxJNA.decrypt_note(
                            output.cipherValue,
                            output.cipherValue.length,
                            confidential.viewSk,
                            confidential.viewSk.length);
            Confidential.PlainValue onePlainValue =
                    RLPCodec.decode(
                            plainBytes,
                            Confidential.PlainValue.class,
                            Web.chainManager.getChainId());

            switch (index) {
                case 0:
                    Assertions.assertEquals(onePlainValue.quatity, Uint64.of(1));
                    index = index + 1;
                    break;
                case 1:
                    Assertions.assertEquals(onePlainValue.quatity, Uint64.of(3));
                    index = index + 1;
                    break;
                case 2:
                    Assertions.assertEquals(onePlainValue.quatity, Uint64.of(5));
                    index = index + 1;
                    break;
                default:
                    Assertions.assertTrue(false);
            }
        }
    }
}
