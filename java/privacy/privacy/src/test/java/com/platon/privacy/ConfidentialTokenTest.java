package com.platon.privacy;

import com.alaya.abi.wasm.WasmEventValues;
import com.alaya.crypto.Hash;
import com.alaya.crypto.Sign;
import com.alaya.protocol.core.methods.response.Log;
import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Uint128;
import com.alaya.rlp.wasm.datatypes.Uint32;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.alaya.tx.WasmContract;
import com.google.common.primitives.Bytes;
import com.platon.privacy.confidential.Confidential;
import com.platon.privacy.confidential.ConfidentialTxJNA;
import com.platon.privacy.contracts.Confidential_token;
import com.platon.privacy.contracts.Confidential_validator;
import org.apache.commons.codec.binary.Hex;
import org.junit.jupiter.api.*;

import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.*;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class  ConfidentialTokenTest {
    private static ConfidentialDeploy.Token token;
    private static Confidential confidential;
    private static List<Confidential.ConfidentialInput> input;
    private static final Map<String, Confidential.OutputInfo> outputInfo = new HashMap<>();
    private static final Map<String, Confidential.MetaDataInfo> metaDataInfo = new HashMap<>();
    private static byte[] authorizedAddress;

    private static byte[] signRawData(byte[] rawData) {
        Sign.SignatureData signatureData = Sign.signMessage(rawData, Web.admin.getEcKeyPair());
        byte v = signatureData.getV()[0];
        v -= 27;
        return Bytes.concat(signatureData.getR(), signatureData.getS(), new byte[] {v});
    }

    private static void fetchOutputFromReceipt(TransactionReceipt receipt){
        String createNoteDetailEvent =
                ConfidentialDeploy.encodeEventName(
                        Confidential_validator.CREATENOTEDETAILEVENT_EVENT.getName());
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(createNoteDetailEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_validator.CREATENOTEDETAILEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                byte[] ownerValue = (byte[]) values.getNonIndexedValues().get(0);
                                Confidential.EncryptedOwner oneOwner =
                                        RLPCodec.decode(
                                                ownerValue,
                                                Confidential.EncryptedOwner.class,
                                                Web.chainManager.getChainId());
                                boolean boolOwner =
                                        false;
                                try {
                                    boolOwner = ConfidentialTxJNA.is_note_owner(
                                            oneOwner.ephemeralPk,
                                            oneOwner.ephemeralPk.length,
                                            oneOwner.signPk,
                                            oneOwner.signPk.length,
                                            confidential.spendPk,
                                            confidential.spendPk.length,
                                            confidential.viewSk,
                                            confidential.viewSk.length);
                                } catch (Exception e) {
                                    e.printStackTrace();
                                }
                                if (!boolOwner) {
                                    try {
                                        throw new Exception("invalid owner");
                                    } catch (Exception e) {
                                        e.printStackTrace();
                                    }
                                }

                                byte[] cipherValue = (byte[]) values.getNonIndexedValues().get(1);

                                byte[] plainBytes =
                                        new byte[0];
                                try {
                                    plainBytes = ConfidentialTxJNA.decrypt_note(
                                            cipherValue,
                                            cipherValue.length,
                                            confidential.viewSk,
                                            confidential.viewSk.length);
                                } catch (Exception e) {
                                    e.printStackTrace();
                                }
                                Confidential.PlainValue onePlainValue =
                                        RLPCodec.decode(
                                                plainBytes,
                                                Confidential.PlainValue.class,
                                                Web.chainManager.getChainId());

                                System.out.println(
                                        "fetchOutputFromReceipt"
                                                + onePlainValue.quatity.value.toString());
                                input.add(
                                        new Confidential.ConfidentialInput(
                                                confidential,
                                                onePlainValue.quatity.value,
                                                onePlainValue.blinding,
                                                oneOwner.ephemeralPk,
                                                oneOwner.signPk));

                                String hash = values.getIndexedValues().get(0);
                                String owner = Hex.encodeHexString(ownerValue);
                                System.out.println(hash);
                                System.out.println(hash.substring(2));
                                System.out.println(owner);
                                Confidential.OutputInfo oneOutputInfo =
                                        new Confidential.OutputInfo(
                                                confidential,
                                                onePlainValue.quatity.value,
                                                onePlainValue.blinding,
                                                oneOwner.ephemeralPk,
                                                oneOwner.signPk,
                                                owner,
                                                hash,
                                                false);
                                outputInfo.put(hash, oneOutputInfo);
                            }
                        });
    }

    public static void getMintEvent(TransactionReceipt receipt) {
        String mintEvent =
                ConfidentialDeploy.encodeEventName(Confidential_token.MINTEVENT_EVENT.getName());
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(mintEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.MINTEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                byte[] oldMintHash = (byte[]) values.getNonIndexedValues().get(0);
                                System.out.println(Hex.encodeHexString(oldMintHash));

                                System.out.println(
                                        values.getNonIndexedValues().get(1).getClass().toString());
                                Uint128 totalMint = (Uint128) values.getNonIndexedValues().get(1);
                                System.out.println(totalMint.toString());
                            }
                        });
    }

    public static void getBurnEvent(TransactionReceipt receipt) {
        String burnEvent =
                ConfidentialDeploy.encodeEventName(Confidential_token.BURNEVENT_EVENT.getName());
        for (Log log : receipt.getLogs()) {
            String name = log.getTopics().get(0);
            if (name.equals(burnEvent)) {
                WasmEventValues values =
                        WasmContract.staticExtractEventParameters(
                                Confidential_token.BURNEVENT_EVENT, log, Web.chainManager.getChainId());

                byte[] oldBurnHash = (byte[]) values.getNonIndexedValues().get(0);
                System.out.println(Hex.encodeHexString(oldBurnHash));

                Uint128 totalBurn = (Uint128) values.getNonIndexedValues().get(1);
                System.out.println(totalBurn.toString());
            }
        }
    }

    public static Map<String, Confidential.OutputInfo> getCreateNoteEvent(
            TransactionReceipt receipt) {
        String createNoteEvent =
                ConfidentialDeploy.encodeEventName(Confidential_token.CREATENOTEEVENT_EVENT.getName());
        Map<String, Confidential.OutputInfo> txOutputInfo = new HashMap<>();
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(createNoteEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.CREATENOTEEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                String indexHash = values.getIndexedValues().get(1);
                                System.out.println(indexHash);

                                Confidential.OutputInfo oneOutputInfo = outputInfo.get(indexHash);
                                Assertions.assertFalse(oneOutputInfo.hasCreateNoteEvent);

                                String indexOwner = values.getIndexedValues().get(0);
                                System.out.println(indexOwner);

                                byte[] ownerValue = (byte[]) values.getNonIndexedValues().get(0);
                                System.out.println(Hex.encodeHexString(ownerValue));

                                byte[] ownerValueHash = Hash.sha3(ownerValue);
                                Assertions.assertEquals(
                                        Hex.encodeHexString(ownerValueHash),
                                        indexOwner.substring(2));

                                Assertions.assertEquals(
                                        oneOutputInfo.owner, Hex.encodeHexString(ownerValue));
                                oneOutputInfo.hasCreateNoteEvent = true;
                                outputInfo.put(indexHash, oneOutputInfo);
                                txOutputInfo.put(indexHash, oneOutputInfo);
                            }
                        });

        return txOutputInfo;
    }

    public static void getDestoryNoteEvent(TransactionReceipt receipt) {
        String destoryNoteEvent =
                ConfidentialDeploy.encodeEventName(Confidential_token.DESTROYNOTEEVENT_EVENT.getName());
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(destoryNoteEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.DESTROYNOTEEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                String indexHash = values.getIndexedValues().get(1);
                                System.out.println(indexHash);

                                Assertions.assertTrue(outputInfo.containsKey(indexHash));
                                Confidential.OutputInfo oneOutputInfo = outputInfo.get(indexHash);
                                Assertions.assertTrue(oneOutputInfo.hasCreateNoteEvent);

                                String indexOwner = values.getIndexedValues().get(0);
                                System.out.println(indexOwner);

                                byte[] ownerValue = (byte[]) values.getNonIndexedValues().get(0);
                                byte[] ownerValueHash = Hash.sha3(ownerValue);
                                Assertions.assertEquals(
                                        Hex.encodeHexString(ownerValueHash),
                                        indexOwner.substring(2));

                                outputInfo.remove(indexHash);
                            }
                        });
    }

    public static void getMetaDataEvent(TransactionReceipt receipt) {
        String metaDataEvent =
                ConfidentialDeploy.encodeEventName(Confidential_token.METADATAEVENT_EVENT.getName());
        receipt.getLogs()
                .forEach(
                        log -> {
                            String name = log.getTopics().get(0);
                            if (name.equals(metaDataEvent)) {
                                WasmEventValues values =
                                        WasmContract.staticExtractEventParameters(
                                                Confidential_token.METADATAEVENT_EVENT,
                                                log,
                                                Web.chainManager.getChainId());

                                String indexHash = values.getIndexedValues().get(0);
                                System.out.println(indexHash);

                                Assertions.assertTrue(metaDataInfo.containsKey(indexHash));
                                Confidential.MetaDataInfo oneMetaData = metaDataInfo.get(indexHash);

                                byte[] metaData = (byte[]) values.getNonIndexedValues().get(0);
                                String strData = new String(metaData);
                                System.out.println(strData);
                                Assertions.assertEquals(oneMetaData.data, strData);

                                metaDataInfo.remove(indexHash);
                            }
                        });
    }

    @BeforeAll
    public static void testDeployContract() throws Exception {
        token = ConfidentialDeploy.deploy();
        confidential = new Confidential(Web.chainManager.getChainId());
        input = new LinkedList<>();
        WasmAddress adminAddress = new WasmAddress(Web.admin.getAddress());
        System.out.println(adminAddress.getAddress());
        authorizedAddress = adminAddress.getValue();
        System.out.println(Hex.encodeHexString(authorizedAddress));
    }

    @Test
    @Order(1)
    public void testRegistry() throws Exception {
        Assertions.assertEquals(Deploy.tokenName, token.confidentialToken.Name().send());
        Assertions.assertEquals(Deploy.tokenSymbol, token.confidentialToken.Symbol().send());
        Assertions.assertEquals(Deploy.scalingFactor, token.confidentialToken.ScalingFactor().send());
        Assertions.assertEquals(Uint128.of(0), token.confidentialToken.TotalSupply().send());
        System.out.println(System.getProperty("java.library.path"));
    }

    @Test
    @Order(2)
    public void testMint() throws Exception {
        Confidential.Mint mint = new Confidential.Mint();
        mint.authorized_address = authorizedAddress;
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(1)));
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(3)));
        mint.output.add(new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(5)));

        byte[] buf = RLPCodec.encode(mint);
        System.out.println(Hex.encodeHexString(buf));

        Confidential.MintExtra extra = new Confidential.MintExtra();
        extra.oldMintHash = new byte[32];
        String outputMeta0 = "mint0";
        extra.metaData.add(outputMeta0.getBytes(StandardCharsets.UTF_8));
        String outputMeta1 = "mint1";
        extra.metaData.add(outputMeta1.getBytes(StandardCharsets.UTF_8));
        byte[] extraData = RLPCodec.encode(extra);

        Confidential.ConfidentialData data = new Confidential.ConfidentialData();
        data.version = Uint32.of(Confidential.currentVersion + mint.tx_type);
        data.confidentialTx = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);
        System.out.println("mint: " + Hex.encodeHexString(data.confidentialTx));
        data.extraData = extraData;
        byte[] rawData = RLPCodec.encode(data);

        Confidential.ConfidentialProof proof = new Confidential.ConfidentialProof();
        proof.data = rawData;
        proof.signature = signRawData(rawData);

        long totalSupply = token.confidentialToken.TotalSupply().send().value().longValue();

        TransactionReceipt receipt = token.confidentialToken.Mint(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        fetchOutputFromReceipt(receipt);

        Assertions.assertEquals(
                totalSupply + 9, token.confidentialToken.TotalSupply().send().getValue().longValue());

        getMintEvent(receipt);
        Map<String, Confidential.OutputInfo> txOutputInfo = getCreateNoteEvent(receipt);

        txOutputInfo.forEach(
                (key, oneInfo) -> {
                    if (oneInfo.quatity.equals(BigInteger.valueOf(1))) {
                        Confidential.MetaDataInfo oneMetaData =
                                new Confidential.MetaDataInfo(oneInfo.hash, outputMeta0);
                        metaDataInfo.put(oneMetaData.hash, oneMetaData);
                    }

                    if (oneInfo.quatity.equals(BigInteger.valueOf(3))) {
                        Confidential.MetaDataInfo oneMetaData =
                                new Confidential.MetaDataInfo(oneInfo.hash, outputMeta1);
                        metaDataInfo.put(oneMetaData.hash, oneMetaData);
                    }
                });

        getMetaDataEvent(receipt);

        Assertions.assertEquals(input.size(), outputInfo.size());
        Assertions.assertEquals(metaDataInfo.size(), 0);
    }

    @Test
    @Order(3)
    public void testBurn() throws Exception {
        long totalSupply = token.confidentialToken.TotalSupply().send().value().longValue();

        Confidential.Burn burn = new Confidential.Burn();
        burn.authorized_address = authorizedAddress;
        burn.input.add(input.get(0));
        input.remove(0);

        byte[] buf = RLPCodec.encode(burn);
        System.out.println(Hex.encodeHexString(buf));

        Confidential.BurnExtra extra = new Confidential.BurnExtra();
        extra.oldBurnHash = new byte[32];
        byte[] extraData = RLPCodec.encode(extra);

        Confidential.ConfidentialData data = new Confidential.ConfidentialData();
        data.version = Uint32.of(Confidential.currentVersion + burn.tx_type);
        data.confidentialTx = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);
        System.out.println("burn: " + Hex.encodeHexString(data.confidentialTx));
        data.extraData = extraData;
        byte[] rawData = RLPCodec.encode(data);

        Confidential.ConfidentialProof proof = new Confidential.ConfidentialProof();
        proof.data = rawData;
        proof.signature = signRawData(rawData);

        TransactionReceipt receipt = token.confidentialToken.Burn(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(
                totalSupply - 1, token.confidentialToken.TotalSupply().send().getValue().longValue());

        getBurnEvent(receipt);
        getDestoryNoteEvent(receipt);

        Assertions.assertEquals(input.size(), outputInfo.size());
        Assertions.assertEquals(metaDataInfo.size(), 0);
    }

    @Test
    @Order(4)
    public void testTransfer() throws Exception {
        Confidential.Transfer transfer = new Confidential.Transfer();
        transfer.authorized_address = authorizedAddress;
        transfer.input.add(input.get(0));
        input.remove(0);
        transfer.output.add(
                new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(3)));

        byte[] buf = RLPCodec.encode(transfer);
        System.out.println(Hex.encodeHexString(buf));

        Confidential.TransferExtra extra = new Confidential.TransferExtra();
        extra.publicOwner = new WasmAddress(Web.admin.getAddress());

        String outputMeta0 = "transfer0";
        extra.metaData.add(outputMeta0.getBytes(StandardCharsets.UTF_8));
        byte[] extraData = RLPCodec.encode(extra);

        Confidential.ConfidentialData data = new Confidential.ConfidentialData();
        data.version = Uint32.of(Confidential.currentVersion + transfer.tx_type);
        data.confidentialTx = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);
        System.out.println("transfer: " + Hex.encodeHexString(data.confidentialTx));
        data.extraData = extraData;
        byte[] rawData = RLPCodec.encode(data);

        Confidential.ConfidentialProof proof = new Confidential.ConfidentialProof();
        proof.data = rawData;
        proof.signature = signRawData(rawData);

        ConfidentialDeploy.minorUpdate(token);

        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        fetchOutputFromReceipt(receipt);

        Map<String, Confidential.OutputInfo> txOutputInfo = getCreateNoteEvent(receipt);
        getDestoryNoteEvent(receipt);

        for (Map.Entry<String, Confidential.OutputInfo> entry : txOutputInfo.entrySet()) {
            Confidential.OutputInfo oneInfo = entry.getValue();

            if (oneInfo.quatity.equals(BigInteger.valueOf(3))) {
                Confidential.MetaDataInfo oneMetaData =
                        new Confidential.MetaDataInfo(oneInfo.hash, outputMeta0);
                metaDataInfo.put(oneMetaData.hash, oneMetaData);
            }
        }
        getMetaDataEvent(receipt);

        Assertions.assertEquals(input.size(), outputInfo.size());
        Assertions.assertEquals(metaDataInfo.size(), 0);
    }

    @Test
    @Order(5)
    public void testTransferDeposit() throws Exception {
        Confidential.Deposit deposit = new Confidential.Deposit();
        deposit.authorized_address = authorizedAddress;
        deposit.input.add(input.get(0));
        input.remove(0);
        deposit.output.add(
                new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(8)));

        byte[] buf = RLPCodec.encode(deposit);
        System.out.println(Hex.encodeHexString(buf));

        Confidential.ConfidentialData data = new Confidential.ConfidentialData();
        data.version = Uint32.of(Confidential.currentVersion + deposit.tx_type);
        data.confidentialTx = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);
        System.out.println("deposit: " + Hex.encodeHexString(data.confidentialTx));

        Confidential.TransferExtra extra = new Confidential.TransferExtra();
        extra.publicOwner = new WasmAddress(Web.admin.getAddress());
        String outputMeta0 = "deposit0";
        extra.depositSignature = signRawData(data.confidentialTx);
        extra.metaData.add(outputMeta0.getBytes(StandardCharsets.UTF_8));

        byte[] extraData = RLPCodec.encode(extra);
        data.extraData = extraData;
        byte[] rawData = RLPCodec.encode(data);

        Confidential.ConfidentialProof proof = new Confidential.ConfidentialProof();
        proof.data = rawData;
        proof.signature = signRawData(rawData);

        ConfidentialDeploy.updateAcl(token);
        token.arc20.Approve(token.acl.GetTokenManager().send(), Uint128.of(30)).send();

        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        fetchOutputFromReceipt(receipt);

        Map<String, Confidential.OutputInfo> txOutputInfo = getCreateNoteEvent(receipt);
        getDestoryNoteEvent(receipt);

        for (Map.Entry<String, Confidential.OutputInfo> entry : txOutputInfo.entrySet()) {
            Confidential.OutputInfo oneInfo = entry.getValue();

            if (oneInfo.quatity.equals(BigInteger.valueOf(8))) {
                Confidential.MetaDataInfo oneMetaData =
                        new Confidential.MetaDataInfo(oneInfo.hash, outputMeta0);
                metaDataInfo.put(oneMetaData.hash, oneMetaData);
            }
        }
        getMetaDataEvent(receipt);

        Assertions.assertEquals(input.size(), outputInfo.size());
        Assertions.assertEquals(metaDataInfo.size(), 0);

        ConfidentialDeploy.updateAcl(token);
    }

    @Test
    @Order(6)
    public void testTransferWithdraw() throws Exception {
        Confidential.Withdraw withdraw = new Confidential.Withdraw();
        withdraw.authorized_address = authorizedAddress;
        withdraw.input.add(input.get(0));
        input.remove(0);
        withdraw.output.add(
                new Confidential.ConfidentialOutput(confidential, BigInteger.valueOf(1)));

        byte[] buf = RLPCodec.encode(withdraw);
        System.out.println(Hex.encodeHexString(buf));

        Confidential.TransferExtra extra = new Confidential.TransferExtra();
        extra.publicOwner = new WasmAddress(Web.admin.getAddress());
        String outputMeta0 = "withdraw0";
        extra.metaData.add(outputMeta0.getBytes(StandardCharsets.UTF_8));
        byte[] extraData = RLPCodec.encode(extra);

        Confidential.ConfidentialData data = new Confidential.ConfidentialData();
        data.version = Uint32.of(Confidential.currentVersion + withdraw.tx_type);
        data.confidentialTx = ConfidentialTxJNA.create_confidential_tx_by_rlp(buf, buf.length);
        System.out.println("withdraw: " + Hex.encodeHexString(data.confidentialTx));
        data.extraData = extraData;
        byte[] rawData = RLPCodec.encode(data);

        Confidential.ConfidentialProof proof = new Confidential.ConfidentialProof();
        proof.data = rawData;
        proof.signature = signRawData(rawData);

        ConfidentialDeploy.majorUpdate(token);
        ConfidentialDeploy.updateAcl(token);

        TransactionReceipt receipt = token.confidentialToken.Transfer(RLPCodec.encode(proof)).send();
        Assertions.assertTrue(receipt.isStatusOK());

        fetchOutputFromReceipt(receipt);

        Map<String, Confidential.OutputInfo> txOutputInfo = getCreateNoteEvent(receipt);
        getDestoryNoteEvent(receipt);

        txOutputInfo.forEach(
                (key, oneInfo) -> {
                    if (oneInfo.quatity.equals(BigInteger.valueOf(1))) {
                        Confidential.MetaDataInfo oneMetaData =
                                new Confidential.MetaDataInfo(oneInfo.hash, outputMeta0);
                        metaDataInfo.put(oneMetaData.hash, oneMetaData);
                    }
                });
        getMetaDataEvent(receipt);

        Assertions.assertEquals(input.size(), outputInfo.size());
        Assertions.assertEquals(metaDataInfo.size(), 0);
    }

    @Test
    @Order(7)
    public void testUpdateMetaData() throws Exception {
        Optional<Confidential.OutputInfo> oneOutputInfo = outputInfo.values().stream().findAny();
        Assertions.assertTrue(oneOutputInfo.isPresent());

        byte[] hash = Hex.decodeHex(oneOutputInfo.get().hash.substring(2));

        String outputMeta0 = "withdraw0";
        byte[] newMateData = outputMeta0.getBytes(StandardCharsets.UTF_8);
        byte[] signature = signRawData(newMateData);

        TransactionReceipt receipt =
                token.confidentialToken.UpdateMetaData(hash, newMateData, signature).send();

        Assertions.assertTrue(receipt.isStatusOK());
        Assertions.assertEquals(receipt.getLogs().size(), 1);

        Confidential.MetaDataInfo oneMetaData =
                new Confidential.MetaDataInfo(oneOutputInfo.get().hash, outputMeta0);
        metaDataInfo.put(oneMetaData.hash, oneMetaData);

        getMetaDataEvent(receipt);
    }
}
