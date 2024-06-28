from ..ragfile import RagFile
from ..metadata import RagFileMetaV1
from ..preprocessing import TextChunker, SplittingStrategy
from transformers import AutoTokenizer, AutoModel
import torch

from typing import List


class RagFileBuilderHF:
    @classmethod
    def new(cls, tokenizer_id: str, model_id: str, metadata_ver: int):
        tokenizer = AutoTokenizer.from_pretrained(tokenizer_id)
        model = AutoModel.from_pretrained(model_id)
        return cls(tokenizer, model, metadata_ver)

    def __init__(
        self,
        tokenizer,
        model,
        metadata_ver,
        splitting_strategy: SplittingStrategy = SplittingStrategy.NONE,
        **kwargs
    ):
        self.tokenizer = tokenizer
        self.model = model
        self.model.eval()
        self.metadata_ver = metadata_ver
        self.chunker = TextChunker(splitting_strategy, **kwargs)

    def create(self, text: str, **metadata_kwargs) -> List[RagFile]:
        chunks = self.chunker.chunk(text)

        ragfiles = []
        for chunk_num, chunk_text in enumerate(chunks):
            model_inputs = self.tokenizer(
                chunk_text, return_tensors="pt", add_special_tokens=True
            )
            token_ids = model_inputs["input_ids"][0].tolist()
            with torch.no_grad():
                embedding = self.model(**model_inputs).pooler_output[0].tolist()
            metadata = RagFileMetaV1.serialize(
                model_id=self.model.name_or_path,
                tokenizer_id=self.tokenizer.name_or_path,
                labels=metadata_kwargs.get("labels", []),
                source_text=text,
                chunk_number=chunk_num,
            )
            rf = RagFile(
                text=chunk_text,
                token_ids=token_ids,
                embedding=embedding,
                metadata=metadata,
                tokenizer_id=self.tokenizer.name_or_path,
                embedding_id=self.model.name_or_path,
            )
            ragfiles.append(rf)
        return ragfiles
