import unittest
from ragfile import RagFile, minhash, io

class TestRagFile(unittest.TestCase):

    def setUp(self):
        self.text1 = "this is a test text"
        self.text2 = "this is another test text"
        self.token_ids1 = [1, 2, 3, 4]
        self.token_ids2 = [1, 2, 5, 6]
        self.embeddings1 = [[0.1, 0.2, 0.3, 0.4]]
        self.embeddings2 = [[0.5, 0.6, 0.7, 0.8]]
        self.extended_metadata = "metadata"
        self.tokenizer_id = "tokenizer1"
        self.embedding_id = "embedding1"
        self.metadata_version = 1
        self.scan_vector1 = [1, 2, 3, 4]
        self.scan_vector2 = [1, 2, 3, 5]
        self.dense_vector1 = [0.1, 0.2, 0.3, 0.4]
        self.dense_vector2 = [0.5, 0.6, 0.7, 0.8]

        self.ragfile1 = RagFile(
            text=self.text1,
            token_ids=self.token_ids1,
            embeddings=self.embeddings1,
            extended_metadata=self.extended_metadata,
            tokenizer_id=self.tokenizer_id,
            embedding_id=self.embedding_id,
            metadata_version=self.metadata_version,
            scan_vector=self.scan_vector1,
            dense_vector=self.dense_vector1,
        )

        self.ragfile2 = RagFile(
            text=self.text2,
            token_ids=self.token_ids2,
            embeddings=self.embeddings2,
            extended_metadata=self.extended_metadata,
            tokenizer_id=self.tokenizer_id,
            embedding_id=self.embedding_id,
            metadata_version=self.metadata_version,
            scan_vector=self.scan_vector2,
            dense_vector=self.dense_vector2
        )

    def test_ragfile_creation(self):
        self.assertEqual(self.ragfile1.text, self.text1)
        self.assertEqual(self.ragfile1.header.scan_vector, self.scan_vector1)
        for dv, expected in zip(self.ragfile1.header.dense_vector, self.dense_vector1):
            self.assertAlmostEqual(dv, expected, delta=1e-3)
        for value, expected in zip(self.ragfile1.embeddings[0], self.embeddings1[0]):
            self.assertAlmostEqual(value, expected, delta=1e-6)
        self.assertEqual(self.ragfile1.extended_metadata, self.extended_metadata)

    def test_jaccard_similarity(self):
        similarity = self.ragfile1.jaccard(self.ragfile2)
        self.assertGreaterEqual(similarity, 0.0)
        self.assertLessEqual(similarity, 1.0)
        similarity = self.ragfile1.jaccard(self.ragfile1)
        self.assertEqual(similarity, 1.0)

    def test_hamming_similarity(self):
        similarity = self.ragfile1.hamming(self.ragfile2)
        self.assertGreaterEqual(similarity, 0.0)
        self.assertLessEqual(similarity, 1.0)
        similarity = self.ragfile1.hamming(self.ragfile1)
        self.assertEqual(similarity, 1.0)

    def test_cosine_similarity(self):
        similarity = self.ragfile1.cosine(self.ragfile2)
        self.assertGreaterEqual(similarity, 0.0)
        self.assertLessEqual(similarity, 1.0)
        similarity = self.ragfile1.cosine(self.ragfile1)
        self.assertEqual(similarity, 1.0)

    def test_match_files(self):
        file_paths = ["path/to/ragfile1", "path/to/ragfile2"]
        file_iter = iter(file_paths)
        results = self.ragfile1.match(file_iter, top_k=2, method="jaccard")
        self.assertIsInstance(results, list)
        self.assertTrue(len(results) <= 2)

        file_iter = iter(file_paths)
        results = self.ragfile1.match(file_iter, top_k=2, method="hamming")
        self.assertIsInstance(results, list)
        self.assertTrue(len(results) <= 2)

    def test_dumps_loads(self):
        rf_ser = io.dumps(self.ragfile1)
        rf = io.loads(rf_ser)
        self.assertEqual(rf.header.dense_vector, self.ragfile1.header.dense_vector)
        self.assertEqual(rf.header.scan_vector, self.ragfile1.header.scan_vector)
        self.assertEqual(rf.header.dense_vector_dim, self.ragfile1.header.dense_vector_dim)
        self.assertEqual(rf.header.scan_vector_dim, self.ragfile1.header.scan_vector_dim)
        self.assertEqual(rf.header.scan_vector_dim, 4)
        self.assertEqual(rf.header.dense_vector_dim, 4)
        self.assertEqual(hash(io.dumps(rf)), hash(io.dumps(rf)))
        self.assertEqual(rf.header.tokenizer_id, self.ragfile1.header.tokenizer_id)

class TestMinHash(unittest.TestCase):

    def test_char(self):
        text = "this is a test text"
        signature = minhash.char(text, ngram=3, permute=128, seed=42)
        self.assertIsInstance(signature, list)
        self.assertEqual(len(signature), 128)

    def test_word(self):
        text = "this is a test text"
        signature = minhash.word(text, ngram=2, permute=256, seed=42)
        self.assertIsInstance(signature, list)
        self.assertEqual(len(signature), 256)

    def test_tokens(self):
        tokens = [1, 2, 3, 4]
        signature = minhash.tokens(tokens, ngram=2, permute=128, seed=42)
        self.assertIsInstance(signature, list)
        self.assertEqual(len(signature), 128)

    def test_merge(self):
        sig1 = [i for i in range(128)]
        sig2 = [i for i in range(128, 256)]
        merged_signature = minhash.merge(sig1, sig2)
        self.assertIsInstance(merged_signature, list)
        self.assertEqual(len(merged_signature), 128)

if __name__ == "__main__":
    unittest.main()
