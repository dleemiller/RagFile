import unittest
from .text_chunker import TextChunker, SplittingStrategy


class TestTextChunker(unittest.TestCase):
    def test_chunk_splitting(self):
        text = "One two three four five"
        chunker = TextChunker(SplittingStrategy.CHUNK, chunk_size=3)
        result = chunker.chunk(text)
        self.assertEqual(result, ["One two three ", "four five"])
        self.assertEqual("".join(result), text)

    def test_sliding_window_splitting(self):
        text = "One two three four five"
        chunker = TextChunker(
            SplittingStrategy.SLIDING_WINDOW, chunk_size=3, step_size=2
        )
        result = chunker.chunk(text)
        self.assertEqual(result, ["One two three ", "three four five"])
        self.assertEqual("".join(result), "One two three three four five")

    def test_paragraph_splitting(self):
        text = "Paragraph one.\n\nParagraph two.\n\nParagraph three."
        chunker = TextChunker(SplittingStrategy.PARAGRAPH)
        result = chunker.chunk(text)
        self.assertEqual(
            result, ["Paragraph one.\n\n", "Paragraph two.\n\n", "Paragraph three."]
        )
        self.assertEqual("".join(result), text)

    def test_sentence_splitting(self):
        text = "This is a sentence. Here's another one! And one more?"
        chunker = TextChunker(SplittingStrategy.SENTENCE)
        result = chunker.chunk(text)
        self.assertEqual(
            result, ["This is a sentence. ", "Here's another one! ", "And one more?"]
        )
        self.assertEqual("".join(result), text)

    def test_word_splitting(self):
        text = "Word1 word2 word3"
        chunker = TextChunker(SplittingStrategy.WORD)
        result = chunker.chunk(text)
        self.assertEqual(result, ["Word1 ", "word2 ", "word3"])
        self.assertEqual("".join(result), text)

    def test_character_splitting(self):
        text = "abc"
        chunker = TextChunker(SplittingStrategy.CHARACTER)
        result = chunker.chunk(text)
        self.assertEqual(result, ["a", "b", "c"])
        self.assertEqual("".join(result), text)

    def test_none_splitting(self):
        text = "No split here."
        chunker = TextChunker(SplittingStrategy.NONE)
        result = chunker.chunk(text)
        self.assertEqual(result, ["No split here."])
        self.assertEqual("".join(result), text)

    def test_invalid_strategy(self):
        with self.assertRaises(ValueError):
            TextChunker("invalid_strategy")


if __name__ == "__main__":
    unittest.main()
