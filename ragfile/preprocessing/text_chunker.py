import re
from more_itertools import chunked, windowed


class SplittingStrategy:
    CHUNK = "chunk"
    SLIDING_WINDOW = "sliding_window"
    PARAGRAPH = "paragraph"
    SENTENCE = "sentence"
    WORD = "word"
    CHARACTER = "character"
    NONE = "none"


class TextChunker:
    def __init__(self, strategy, chunk_size=256, step_size=128):
        self.chunk_size = chunk_size
        self.step_size = step_size
        self._compile_regex()
        self.splitting_function = self._select_splitting_function(strategy)

    def _compile_regex(self):
        # Precompile all regex patterns used across different methods
        self._word_pattern = re.compile(r"\S+\s*")
        self._paragraph_pattern = re.compile(r"(\n\s*\n)")
        self._sentence_pattern = re.compile(r"(\S.*?[.!?])(\s*)")

    def _select_splitting_function(self, strategy):
        strategy_function_map = {
            SplittingStrategy.CHUNK: self._split_chunk,
            SplittingStrategy.SLIDING_WINDOW: self._split_sliding_window,
            SplittingStrategy.PARAGRAPH: self._split_paragraph,
            SplittingStrategy.SENTENCE: self._split_sentence,
            SplittingStrategy.WORD: self._split_word,
            SplittingStrategy.CHARACTER: self._split_character,
            SplittingStrategy.NONE: self._split_none,
        }
        if strategy in strategy_function_map:
            return strategy_function_map[strategy]
        else:
            raise ValueError(f"Invalid splitting strategy: {strategy}")

    def chunk(self, text):
        return self.splitting_function(text)

    def _split_chunk(self, text):
        words = self._word_pattern.findall(text)
        chunked_words = chunked(words, self.chunk_size)
        return ["".join(chunk) for chunk in chunked_words]

    def _split_sliding_window(self, text):
        words = self._word_pattern.findall(text)
        sliding_windows = windowed(
            words, n=self.chunk_size, step=self.step_size, fillvalue=""
        )
        return ["".join(window) for window in sliding_windows]

    def _split_paragraph(self, text):
        paragraphs = self._paragraph_pattern.split(text)
        return [
            "".join(paragraphs[i : i + 2])
            for i in range(0, len(paragraphs), 2)
            if paragraphs[i].strip()
        ]

    def _split_sentence(self, text):
        parts = self._sentence_pattern.findall(text)
        return ["".join(part) for part in parts]

    def _split_word(self, text):
        words = self._word_pattern.findall(text)
        return words

    def _split_character(self, text):
        return list(text)

    def _split_none(self, text):
        return [text]
