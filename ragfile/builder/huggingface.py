from ..ragfile import RagFile
from ..metadata import RagFileMetaV1
from ..preprocessing import TextChunker, SplittingStrategy
from transformers import AutoTokenizer, AutoModel
import torch

from typing import List


class RagFileBuilderHF:
    @classmethod
    def new(
        cls,
        tokenizer_id: str,
        model_id: str,
        metadata_ver: int,
        splitting_strategy: SplittingStrategy = SplittingStrategy.NONE,
    ):
        device = "cuda" if torch.cuda.is_available() else "cpu"
        tokenizer = AutoTokenizer.from_pretrained(tokenizer_id)
        model = AutoModel.from_pretrained(model_id).to(device)
        return cls(
            tokenizer, model, metadata_ver, splitting_strategy=splitting_strategy
        )

    def __init__(
        self,
        tokenizer,
        model,
        metadata_ver,
        splitting_strategy: SplittingStrategy = SplittingStrategy.NONE,
        **kwargs,
    ):
        self.tokenizer = tokenizer
        self.model = model
        self.model.eval()  # Ensure model is in evaluation mode
        self.metadata_ver = metadata_ver
        self.chunker = TextChunker(splitting_strategy, **kwargs)

    def create(self, text: str, **metadata_kwargs) -> List[RagFile]:
        chunks = self.chunker.chunk(text)
        model_inputs = self.tokenizer(
            chunks,
            return_tensors="pt",
            add_special_tokens=True,
            truncation=True,
            max_length=512,
            padding=True,
        )
        # Move the model inputs to the same device as the model
        model_inputs = {
            key: val.to(self.model.device) for key, val in model_inputs.items()
        }
        token_ids = model_inputs["input_ids"]

        # get the tokens for minhashing
        # no truncation, special tokens or padding
        token_list = self.tokenizer(
            text,
            truncation=False,
            add_special_tokens=False,
            padding=False,
            return_tensors=None,
            return_token_type_ids=False,
            return_attention_mask=False,
        )

        with torch.no_grad():
            embeddings = self.model(**model_inputs).pooler_output

        metadata = RagFileMetaV1.serialize(**metadata_kwargs)
        rf = RagFile(
            text=text,
            token_ids=token_list["input_ids"],
            embedding_id=self.model.name_or_path,
            tokenizer_id=self.tokenizer.name_or_path,
            embeddings=embeddings.tolist(),
            extended_metadata=metadata,
            metadata_version=self.metadata_ver,
        )
        return rf
