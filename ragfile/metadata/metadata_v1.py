import ctypes
import hashlib
import time
import base64
from functools import partial


class RagFileMetaV1(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("model_id", ctypes.c_char * 64),
        ("tokenizer_id", ctypes.c_char * 64),
        ("labels", ctypes.c_ushort * 16),
        ("sourcefile_name", ctypes.c_char * 128),
        ("sourcefile_hash", ctypes.c_char * 64),
        ("chunk_number", ctypes.c_int),
        ("creation_timestamp", ctypes.c_ulong),
    ]

    @classmethod
    def serialize(
        cls,
        model_id: str,
        tokenizer_id: str,
        labels: list,
        source_text: str,
        chunk_number: int,
        sourcefile_name: str = "",
    ) -> str:
        instance = cls()
        instance.model_id = model_id.encode("utf-8")[:64]
        instance.tokenizer_id = tokenizer_id.encode("utf-8")[:64]
        labels_array = (ctypes.c_ushort * 16)(*labels[:16])
        instance.labels = labels_array
        instance.sourcefile_name = sourcefile_name.encode("utf-8")[:128]

        # Calculate the source file hash from the source text
        hasher = hashlib.sha256()
        hasher.update(source_text.encode("utf-8"))
        instance.sourcefile_hash = hasher.hexdigest().encode("utf-8")[:64]

        instance.chunk_number = chunk_number
        instance.creation_timestamp = int(time.time())

        # Serialize to bytes and encode to base64 string
        return base64.b64encode(bytes(instance)).decode("utf-8")

    @classmethod
    def deserialize(cls, metadata_str: str) -> "RagFileMetaV1":
        # Decode base64 string to bytes and deserialize
        metadata_bytes = base64.b64decode(metadata_str.encode("utf-8"))
        return cls.from_buffer_copy(metadata_bytes)
