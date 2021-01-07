package com.platon.privacy;

import com.alaya.protocol.core.methods.response.TransactionReceipt;
import com.alaya.protocol.exceptions.TransactionException;
import com.alaya.rlp.wasm.RLPCodec;
import com.alaya.rlp.wasm.datatypes.Int128;
import com.alaya.rlp.wasm.datatypes.Uint128;
import com.alaya.rlp.wasm.datatypes.WasmAddress;
import com.platon.privacy.contracts.Storage;
import org.junit.jupiter.api.*;

import java.math.BigInteger;

@TestMethodOrder(MethodOrderer.OrderAnnotation.class)
public class StorageTest {

    private static final byte[] one = BigInteger.ONE.toByteArray();
    private static final byte[] two = BigInteger.valueOf(2).toByteArray();
    private static final byte[] three = BigInteger.valueOf(3).toByteArray();
    private static final byte[] four = BigInteger.valueOf(4).toByteArray();
    private static final byte[] five = BigInteger.valueOf(5).toByteArray();
    private static final byte[] oneHash =
            new byte[] {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1
            };
    private static final byte[] twoHash =
            new byte[] {
                2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1
            };
    private static final byte[] threeHash =
            new byte[] {
                3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1
            };
    private static final byte[] fourHash =
            new byte[] {
                4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1
            };
    private static final byte[] fiveHash =
            new byte[] {
                5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 1
            };
    private static final WasmAddress adminAddress = new WasmAddress(Web.admin.getAddress());
    private static Storage storage;

    @BeforeAll
    public static void deployContract() throws Exception {
        storage =
                Storage.deploy(
                                Web.chainManager.getWeb3j(),
                                Web.txManager,
                                Web.chainManager.getGasProvider(),
                                Web.chainManager.getChainId())
                        .send();
    }

    @Test
    @Order(1)
    public void testIllegalUpdateNote() {
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.UpdateNotes(new byte[0]).send();
                });
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.Approve(new byte[0]).send();
                });
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.Mint(new byte[0]).send();
                });
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.Burn(new byte[0]).send();
                });
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    storage.GetApproval(new byte[32]).send();
                });
        Assertions.assertThrows(
                ArrayIndexOutOfBoundsException.class,
                () -> {
                    storage.GetNote(new byte[32]).send();
                });
    }

    @Test
    @Order(2)
    public void testCreateRegistry() throws Exception {
        TransactionReceipt receipt = storage.CreateRegistry(true).send();
        Assertions.assertTrue(receipt.isStatusOK());
    }

    @Test
    @Order(3)
    public void testRecreateRegistry() {
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.CreateRegistry(true).send();
                });
    }

    @Test
    @Order(4)
    public void testUpdateNote() throws Exception {
        Validator.OutputNotes[] outputs =
                new Validator.OutputNotes[] {
                    new Validator.OutputNotes(one, oneHash, one),
                    new Validator.OutputNotes(two, twoHash, two)
                };
        Validator.TransferResult result =
                new Validator.TransferResult(
                        new Validator.InputNotes[0],
                        outputs,
                        new WasmAddress(BigInteger.ZERO),
                        Int128.of(12),
                        adminAddress.getValue());
        TransactionReceipt receipt = storage.UpdateNotes(RLPCodec.encode(result)).send();
        Assertions.assertTrue(receipt.isStatusOK());
        Validator.InputNotes[] inputs =
                new Validator.InputNotes[] {
                    new Validator.InputNotes(one, oneHash), new Validator.InputNotes(two, twoHash)
                };
        outputs =
                new Validator.OutputNotes[] {
                    new Validator.OutputNotes(three, threeHash, three),
                    new Validator.OutputNotes(four, fourHash, four)
                };
        result =
                new Validator.TransferResult(
                        inputs,
                        outputs,
                        new WasmAddress(BigInteger.ZERO),
                        Int128.of(12),
                        adminAddress.getValue());
        final byte[] stream = RLPCodec.encode(result);
        receipt = storage.UpdateNotes(stream).send();
        Assertions.assertTrue(receipt.isStatusOK());

        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    storage.UpdateNotes(stream).send();
                });
    }

    @Test
    @Order(5)
    public void testApprove() throws Exception {
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    Validator.ApproveResult result =
                            new Validator.ApproveResult(oneHash, one, adminAddress.getValue());
                    storage.Approve(RLPCodec.encode(result)).send();
                });
        Validator.ApproveResult result =
                new Validator.ApproveResult(fourHash, four, adminAddress.getValue());
        TransactionReceipt receipt = storage.Approve(RLPCodec.encode(result)).send();
        Assertions.assertTrue(receipt.isStatusOK());
    }

    @Test
    @Order(6)
    public void testMint() throws Exception {

        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    Validator.MintResult result =
                            new Validator.MintResult(
                                    one,
                                    one,
                                    Uint128.of(10),
                                    new Validator.OutputNotes[] {
                                        new Validator.OutputNotes(four, fourHash, four)
                                    },
                                    adminAddress.getValue());
                    storage.Mint(RLPCodec.encode(result)).send();
                });
        Validator.MintResult result =
                new Validator.MintResult(
                        one,
                        one,
                        Uint128.of(10),
                        new Validator.OutputNotes[] {
                            new Validator.OutputNotes(five, fiveHash, five)
                        },
                        adminAddress.getValue());
        TransactionReceipt receipt = storage.Mint(RLPCodec.encode(result)).send();
        Assertions.assertTrue(receipt.isStatusOK());
    }

    @Test
    @Order(7)
    public void testBurn() throws Exception {
        Assertions.assertThrows(
                TransactionException.class,
                () -> {
                    Validator.BurnResult result =
                            new Validator.BurnResult(
                                    one,
                                    one,
                                    Uint128.of(10),
                                    new Validator.InputNotes[] {
                                        new Validator.InputNotes(one, oneHash)
                                    },
                                    adminAddress.getValue());
                    storage.Approve(RLPCodec.encode(result)).send();
                });

        Validator.BurnResult result =
                new Validator.BurnResult(
                        one,
                        one,
                        Uint128.of(10),
                        new Validator.InputNotes[] {new Validator.InputNotes(five, fiveHash)},
                        adminAddress.getValue());

        TransactionReceipt receipt = storage.Burn(RLPCodec.encode(result)).send();
        Assertions.assertTrue(receipt.isStatusOK());
    }
}
