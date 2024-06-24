import unittest
import ragfile


class TestRagFile(unittest.TestCase):

    def setUp(self):
        self.text = "Sample text"
        self.token_ids = [1, 2, 3, 4]
        self.embedding = [0.1, 0.2, 0.3, 0.4]
        self.metadata = "metadata"
        self.tokenizer_id_hash = "some-huggingface/tokenizer_id"
        self.embedding_id_hash = "some/embedding_id_str"
        self.metadata_version = 1

        self.ragfile = ragfile.RagFile(
            text=self.text,
            token_ids=self.token_ids,
            embedding=self.embedding,
            metadata=self.metadata,
            tokenizer_id=self.tokenizer_id_hash,
            embedding_id=self.embedding_id_hash,
            metadata_version=self.metadata_version,
        )

    def assertListAlmostEqual(self, list1, list2, places=7):
        self.assertEqual(len(list1), len(list2))
        for a, b in zip(list1, list2):
            self.assertAlmostEqual(a, b, places=places)

    def test_create_ragfile(self):
        self.assertEqual(self.ragfile.text, self.text)
        self.assertListAlmostEqual(self.ragfile.embedding, self.embedding)
        self.assertEqual(self.ragfile.metadata, self.metadata)
        self.assertEqual(self.ragfile.header.version, self.metadata_version)

    def test_dumps_and_loads(self):
        serialized = ragfile.dumps(self.ragfile)
        deserialized = ragfile.loads(serialized)

        self.assertEqual(deserialized.text, self.text)
        self.assertListAlmostEqual(deserialized.embedding, self.embedding)
        self.assertEqual(deserialized.metadata, self.metadata)
        self.assertEqual(deserialized.header.version, self.metadata_version)

    def test_load_and_dump_file(self):
        with open("test.rag", "wb") as f:
            ragfile.dump(self.ragfile, f)

        with open("test.rag", "rb") as f:
            loaded_ragfile = ragfile.load(f)

        self.assertEqual(loaded_ragfile.text, self.text)
        self.assertListAlmostEqual(loaded_ragfile.embedding, self.embedding)
        self.assertEqual(loaded_ragfile.metadata, self.metadata)
        self.assertEqual(loaded_ragfile.header.version, self.metadata_version)

    def test_jaccard_similarity(self):
        serialized = ragfile.dumps(self.ragfile)
        deserialized = ragfile.loads(serialized)

        similarity = self.ragfile.jaccard(deserialized)
        self.assertAlmostEqual(similarity, 1.0)

    def test_cosine_similarity(self):
        serialized = ragfile.dumps(self.ragfile)
        deserialized = ragfile.loads(serialized)

        similarity = self.ragfile.cosine(deserialized)
        self.assertAlmostEqual(similarity, 1.0)

    # New test cases start here
    def test_missing_required_arguments(self):
        with self.assertRaises(ValueError):
            ragfile.RagFile(
                text=self.text,
                token_ids=self.token_ids,
                embedding=self.embedding,
                tokenizer_id=self.tokenizer_id_hash,
                # embedding_id is missing
            )

    def test_token_id_length_less_than_3(self):
        with self.assertRaises(ValueError):
            ragfile.RagFile(
                text=self.text,
                token_ids=[1, 2],  # Less than 3 elements
                embedding=self.embedding,
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

    def test_empty_string_and_lists(self):
        with self.assertRaises(ValueError):
            ragfile.RagFile(
                text="",
                token_ids=[],
                embedding=[],
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

    def test_incorrect_types_for_arguments(self):
        with self.assertRaises(TypeError):
            ragfile.RagFile(
                text=self.text,
                token_ids="invalid",  # Should be a list of integers
                embedding=self.embedding,
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

        with self.assertRaises(TypeError):
            ragfile.RagFile(
                text=self.text,
                token_ids=self.token_ids,
                embedding="invalid",  # Should be a list of floats
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

        with self.assertRaises(TypeError):
            ragfile.RagFile(
                text=self.text,
                token_ids=[1, 2, "invalid"],  # Should be a list of integers
                embedding=self.embedding,
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

        with self.assertRaises(TypeError):
            ragfile.RagFile(
                text=self.text,
                token_ids=self.token_ids,
                embedding=[0.1, 0.2, "invalid"],  # Should be a list of floats
                tokenizer_id=self.tokenizer_id_hash,
                embedding_id=self.embedding_id_hash,
            )

    def test_optional_metadata(self):
        rf = ragfile.RagFile(
            text=self.text,
            token_ids=self.token_ids,
            embedding=self.embedding,
            tokenizer_id=self.tokenizer_id_hash,
            embedding_id=self.embedding_id_hash,
            # metadata and metadata_version are optional
        )
        self.assertIsNone(rf.metadata)
        self.assertEqual(rf.header.version, 1)

    def test_large_input_data(self):
        large_text = "A" * 10000
        large_tokens = list(range(10000))
        large_embedding = [0.1] * 10000
        rf = ragfile.RagFile(
            text=large_text,
            token_ids=large_tokens,
            embedding=large_embedding,
            tokenizer_id=self.tokenizer_id_hash,
            embedding_id=self.embedding_id_hash,
        )
        self.assertEqual(rf.text, large_text)
        self.assertEqual(len(rf.embedding), len(large_embedding))


if __name__ == "__main__":
    unittest.main()
