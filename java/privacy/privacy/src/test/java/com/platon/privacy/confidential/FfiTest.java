package com.platon.privacy.confidential;

import org.bouncycastle.util.encoders.Hex;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.nio.charset.Charset;

public class FfiTest {
    @Test
    void testFfi() {

        ConfidentialLibrary ins = ConfidentialLibrary.INSTANCE;
        byte[] sk = new byte[10];
        sk[1] = 1;
        ExternError error = new ExternError();
        ByteBuffer.ByValue value = ins.create_keypair(sk, 10, error);
        Assertions.assertEquals(value.len, 68);
        Assertions.assertEquals(error.code, 0);
    }

    void testBuildVerify(String json) {
        ConfidentialLibrary ins = ConfidentialLibrary.INSTANCE;
        ExternError error = new ExternError();
        ByteBuffer.ByValue txLog =
                ins.create_confidential_tx(
                        json.getBytes(Charset.defaultCharset()),
                        json.getBytes(Charset.defaultCharset()).length,
                        error);
        if (0 != error.code) {
            String errorInfo = error.message.getString(0);
            System.out.println(errorInfo);
        }
        Assertions.assertEquals(error.code, 0);
        System.out.println(Hex.toHexString(txLog.data.getByteArray(0, (int) txLog.len)));
        ins.confidential_tx_verify(
                txLog.data.getByteArray(0, (int) txLog.len), (int) txLog.len, error);

        Assertions.assertEquals(error.code, 0);
        Assertions.assertNotEquals(txLog, 0);
    }

    @Test
    void testCase() {
        String json =
                "{\n"
                        + "        \"tx_type\": 1,\n"
                        + "        \"input\":[\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"566a57f8dac3ce75177ad9f47cca5c8f7c159a19b7dffa7199a08edc43b8d777\",\n"
                        + "                \"sign_pk\": \"be73cefa7f638a03d2e14ad7ec04fbb1906a84579cfb8176c2faa599534ac36f\",\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            },\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"fc6d651a50268da1726e5ecb75c0a8926cc91694f29df9d2cd22ce8ebcf54213\",\n"
                        + "                \"sign_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"quantity\": 4,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            }\n"
                        + "        ],\n"
                        + "        \"output\": [\n"
                        + "            {\n"
                        + "                \"quantity\": 4,\n"
                        + "                \"view_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"spend_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\"\n"
                        + "            },\n"
                        + "            {\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"view_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"spend_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\"\n"
                        + "            }\n"
                        + "        ]\n"
                        + "        \"authorized_address\": \"0affa2249e8200e43709001363b9dce05289e0f1\"\n"
                        + "    }";

        // testBuildVerify(json);
        String mint =
                "{\n"
                        + "        \"tx_type\": 2,\n"
                        + "        \"output\": [\n"
                        + "            {\n"
                        + "                \"quantity\": 4,\n"
                        + "                \"view_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"spend_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\"\n"
                        + "            },\n"
                        + "            {\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"view_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"spend_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\"\n"
                        + "            }\n"
                        + "        ],\n"
                        + "        \"authorized_address\": \"0affa2249e8200e43709001363b9dce05289e0f1\"\n"
                        + "    }";
        testBuildVerify(mint);

        String destroy =
                "{\n"
                        + "        \"tx_type\": 3,\n"
                        + "        \"input\":[\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"566a57f8dac3ce75177ad9f47cca5c8f7c159a19b7dffa7199a08edc43b8d777\",\n"
                        + "                \"sign_pk\": \"be73cefa7f638a03d2e14ad7ec04fbb1906a84579cfb8176c2faa599534ac36f\",\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            },\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"fc6d651a50268da1726e5ecb75c0a8926cc91694f29df9d2cd22ce8ebcf54213\",\n"
                        + "                \"sign_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"quantity\": 4,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            }\n"
                        + "        ],\n"
                        + "        \"authorized_address\": \"0affa2249e8200e43709001363b9dce05289e0f1\"\n"
                        + "    }";
        testBuildVerify(destroy);

        String withdraw =
                "{\n"
                        + "        \"tx_type\": 5,\n"
                        + "        \"input\":[\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"566a57f8dac3ce75177ad9f47cca5c8f7c159a19b7dffa7199a08edc43b8d777\",\n"
                        + "                \"sign_pk\": \"be73cefa7f638a03d2e14ad7ec04fbb1906a84579cfb8176c2faa599534ac36f\",\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0100000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            },\n"
                        + "            {\n"
                        + "                \"ephemeral_pk\": \"fc6d651a50268da1726e5ecb75c0a8926cc91694f29df9d2cd22ce8ebcf54213\",\n"
                        + "                \"sign_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"quantity\": 4,\n"
                        + "                \"blinding\": \"0100000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"view_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\",\n"
                        + "                \"spend_sk\": \"0400000000000000000000000000000000000000000000000000000000000000\"\n"
                        + "            }\n"
                        + "        ],\n"
                        + "        \"output\": [\n"
                        + "            {\n"
                        + "                \"quantity\": 1,\n"
                        + "                \"view_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\",\n"
                        + "                \"spend_pk\": \"0affa2249e8200e43709001363b9dce05289e0f1436630ba7bb04924438e9862\"\n"
                        + "            }\n"
                        + "        ],\n"
                        + "        \"authorized_address\": \"0affa2249e8200e43709001363b9dce05289e0f1\"\n"
                        + "    }";
        testBuildVerify(withdraw);
    }

    @Test
    void testNoteOwner() {
        byte[] ephemeral_pk_str =
                Hex.decode("566a57f8dac3ce75177ad9f47cca5c8f7c159a19b7dffa7199a08edc43b8d777");
        byte[] sign_pk_str =
                Hex.decode("be73cefa7f638a03d2e14ad7ec04fbb1906a84579cfb8176c2faa599534ac36f");
        byte[] spend_pk_str =
                Hex.decode("e2f2ae0a6abc4e71a884a961c500515f58e30b6aa582dd8db6a65945e08d2d76");
        byte[] view_sk_str =
                Hex.decode("0100000000000000000000000000000000000000000000000000000000000000");
        ExternError error = new ExternError();
        ConfidentialLibrary.INSTANCE.is_note_owner(
                ephemeral_pk_str,
                ephemeral_pk_str.length,
                sign_pk_str,
                sign_pk_str.length,
                spend_pk_str,
                spend_pk_str.length,
                view_sk_str,
                view_sk_str.length,
                error);
        Assertions.assertEquals(error.code, 0);
    }

    @Test
    void testNoteDecrypt() {
        byte[] cipher_str =
                Hex.decode(
                        "24a1e5db0674955057732c174782a46d1585d41876232bdaa6b1153ac124267e6d72980232341e10638207be8c6059903fd3d5f4be3e66d8e03282e34b7bd2e905c92beb9cbacbb154d952759d8ec78ed8580e6bc7672c55bee2e17c65bdb45fb028e59e");
        byte[] view_sk_str =
                Hex.decode("0100000000000000000000000000000000000000000000000000000000000000");
        ExternError error = new ExternError();
        ConfidentialLibrary.INSTANCE.decrypt_note(
                cipher_str, cipher_str.length, view_sk_str, view_sk_str.length, error);
        Assertions.assertEquals(error.code, 0);
    }
}
