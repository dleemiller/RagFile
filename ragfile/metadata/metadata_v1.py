import ctypes
import hashlib
import time
import base64
from functools import partial


class RagFileMetaV1(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("labels", ctypes.c_ushort * 16),
        ("dataset_name", ctypes.c_char * 128),
        ("dataset_row_id", ctypes.c_char * 16),
        ("sourcefile_name", ctypes.c_char * 128),
        ("sourcefile_hash", ctypes.c_char * 64),
        ("chunk_number", ctypes.c_int),
        ("creation_timestamp", ctypes.c_ulong),
    ]

    @classmethod
    def serialize(
        cls,
        labels: list = [],
        source_text: str = "",
        chunk_number: int = 0,
        dataset_name: str = "",
        dataset_row_id: str = "",
        sourcefile_name: str = "",
    ) -> str:
        instance = cls()
        labels_array = (ctypes.c_ushort * 16)(*labels[:16])
        instance.labels = labels_array
        instance.sourcefile_name = sourcefile_name.encode("utf-8")[:128]
        instance.dataset_name = dataset_name.encode("utf-8")[:128]
        instance.dataset_row_id = dataset_row_id.encode("utf-8")[:16]

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
