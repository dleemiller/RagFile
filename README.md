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

rf = ragfile.RagFile(
    text="Sample text",
    token_ids=[1, 2, 3, 4],  # token ids
    embedding=[0.1, 0.2, 0.3, 0.4],
    metadata="metadata",  # serialized metadata
    tokenizer_id="123",
    embedding_id="456",
    metadata_version=1
)
```

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

## Contributing

Contributions are welcome! Please open an issue or submit a pull request on GitHub.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

