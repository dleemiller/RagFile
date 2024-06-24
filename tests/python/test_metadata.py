import unittest
import hashlib
from ragfile.metadata import RagFileMetaV1


class TestRagFileMetaV1(unittest.TestCase):

    def test_serialize_and_deserialize(self):
        model_id = "example_model_id"
        tokenizer_id = "example_tokenizer_id"
        labels = [1, 2, 3, 4]
        source_text = "This is the content of the source text used for hashing."
        chunk_number = 1

        metadata_b64 = RagFileMetaV1.serialize(
            model_id=model_id,
            tokenizer_id=tokenizer_id,
            labels=labels,
            source_text=source_text,
            chunk_number=chunk_number,
        )
        assert isinstance(metadata_b64, str)

        deserialized_metadata = RagFileMetaV1.deserialize(metadata_b64)

        self.assertEqual(
            deserialized_metadata.model_id.decode("utf-8").rstrip(chr(0)), model_id
        )
        self.assertEqual(
            deserialized_metadata.tokenizer_id.decode("utf-8").rstrip(chr(0)),
            tokenizer_id,
        )
        self.assertEqual(list(deserialized_metadata.labels)[: len(labels)], labels)
        self.assertEqual(
            deserialized_metadata.sourcefile_name.decode("utf-8").rstrip(chr(0)), ""
        )

        hasher = hashlib.sha256()
        hasher.update(source_text.encode("utf-8"))
        expected_hash = hasher.hexdigest().encode("utf-8")[:64]
        self.assertEqual(
            deserialized_metadata.sourcefile_hash.decode("utf-8").rstrip(chr(0)),
            expected_hash.decode("utf-8"),
        )

        self.assertEqual(deserialized_metadata.chunk_number, chunk_number)
        self.assertIsInstance(deserialized_metadata.creation_timestamp, int)


if __name__ == "__main__":
    unittest.main()
