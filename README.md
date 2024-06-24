# RagFile

RagFile is a Python package designed for using flat files to perform Retrieval-Augmented Generation (RAG). The specification involves a binary format with a header that contains a MinHash signature over tokens from a tokenizer. Using a coarse retrieval, the documents can then be re-ranked using the stored embedding.

The purpose of RagFile is to create a lightweight, simple, and fast interface for performing RAG when vector stores and managing indexes are too heavyweight. This library aims to provide a load/dump style interface for storing files and (TODO) implements query methods for a MinHash/reranking hybrid search.

## Features

- Create and manipulate RagFile objects.
- Load and dump RagFile objects from and to files and strings.
- Compute Jaccard and Cosine similarities between RagFile objects.
- Efficient MinHash implementation for similarity measures.

## TODO

- Implement query and scan functions

## Installation

### From PyPI (Not yet available)

```
pip install ragfile
```

### From Source

Clone the repository and install:

```
git clone https://github.com/dleemiller/ragfile.git
cd ragfile
python setup.py install
```

## Usage

### Creating a RagFile

```
import ragfile
from ragfile.metadata import RagFileMetaV1

# Example source text for generating metadata
source_text = "This is the content of the source text used for hashing."

# Create metadata
metadata = RagFileMetaV1.serialize(
    model_id="example_model_id",
    tokenizer_id="example_tokenizer_id",
    labels=[1, 2, 3, 4],
    source_text=source_text,
    chunk_number=1
)

# Create a RagFile object
rf = ragfile.RagFile(
    text="Sample text",
    token_ids=[1, 2, 3, 4],  # token ids
    embedding=[0.1, 0.2, 0.3, 0.4],
    metadata=metadata,  # serialized metadata
    tokenizer_id="some-tokenizer-identifier", # these ids must be used consistently across files
    embedding_id="some-embedding-identifier", # use the same tokenizer/embedding models for all files
    metadata_version=1
)
```

Note that `tokenizer_id` and `embedding_id` are metadata that are hashed and validated when comparisons are made.
They do not guarantee that you consistently used the same tokenizer and embedding.
They must be matched between files for comparisons to be made, and are validated when using compare functions.
When using scan() and query(), files created using different tokenizers and embedding models according to the ids will be skipped.


### Dumping and Loading RagFile

```
# To/from a file
with open("sample.rag", "wb") as f:
    ragfile.dump(rf, f)

with open("sample.rag", "rb") as f:
    loaded_rf = ragfile.load(f)

# To/from a byte string
rf_string = ragfile.dumps(rf)
loaded_rf = ragfile.loads(rf_string)
```

### Computing Similarities

```
similarity_jaccard = rf.jaccard(loaded_rf)  # MinHash similarity score
similarity_cosine = rf.cosine(loaded_rf)  # Cosine similarity of embedding
```

### Using Tokenizer Library

This demonstrates using it with the tokenizer library to create embeddings.
It is not required to use the token ids associated with the embedding with the minhashing algorithm in the ragfile.
However, for any two files to be compared, you must use the same tokenizer across files and the same embedding model across files.
The embedding id and tokenizer id will be used to record which tokenizer/embedding that was used, so ensure that your id strings are set consistently.

```
from transformers import AutoTokenizer, AutoModel
import torch

# Load tokenizer and model
tokenizer = AutoTokenizer.from_pretrained("sentence-transformers/all-MiniLM-L6-v2")
model = AutoModel.from_pretrained("sentence-transformers/all-MiniLM-L6-v2")

# Example sentence
sentence = "A person on a horse jumps over a broken down airplane."

# Tokenize sentence
tokenized = tokenizer(sentence, return_tensors='pt', truncation=True, padding=True)
token_ids = tokenized['input_ids'].squeeze().tolist()
token_ids = [id for id in token_ids if id != tokenizer.pad_token_id]  # Remove padding

# Generate embedding
with torch.no_grad():
    outputs = model(**tokenized)
    embedding = outputs.last_hidden_state.mean(dim=1).squeeze().numpy()

# Create metadata
metadata = RagFileMetaV1.serialize(
    model_id="sentence-transformers/all-MiniLM-L6-v2",
    tokenizer_id="sentence-transformers/all-MiniLM-L6-v2",
    labels=[1, 2, 3, 4],  # any labels (ints) to apply
    source_text=sentence,  # only for computing sha256 hash
    chunk_number=1  # record chunk number if chunking
)

# Create a RagFile object with tokenized data and generated embedding
rf = ragfile.RagFile(
    text=sentence,
    token_ids=token_ids,
    embedding=embedding.tolist(),
    metadata=metadata,
    tokenizer_id="123",
    embedding_id="456",
    metadata_version=1
)
```

## Contributing

Contributions are welcome! Please open an issue or submit a pull request on GitHub.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

