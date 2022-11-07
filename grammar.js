module.exports = grammar({
  name: 'note',

  externals: $ => [
    $.bold_open,
    $.italic_open,
    $.strikethrough_open,
    $.underline_open,
    $.verbatim_open,
    $.inline_math_open,

    $.bold_close,
    $.italic_close,
    $.strikethrough_close,
    $.underline_close,
    $.verbatim_close,
    $.inline_math_close,

    $._space,
    $._blank_line,

    $.tag_end,

    $.word,

    $._, // none
  ],

  // inline: $ => [
  //   $.word
  // ],

  // // https://github.com/tree-sitter/tree-sitter/pull/939
  // precedences: _ => [
  //   ['document_directive', 'body_directive'],
  //   ['special', 'immediate', 'non-immediate'],
  // ],

  rules: {
    document: $ => repeat1(
      choice(
        $.paragraph,
        $._blank_line
      )
    ),

    paragraph: $ => prec.right(0,
      repeat1(
        choice(
          $.word,
          $.bold,
          $.italic,
          $.strikethrough,
          $.underline,
          $.verbatim,
          $.inline_math,
        )
      ),
    ),

    bold: $ => prec.right(0,
      seq(
        alias($.bold_open, "open"),
        repeat1(
          choice(
            $.word,
            $.italic,
            $.underline,
            $.strikethrough,
            $.verbatim,
          ),
        ),
        alias($.bold_close, "close")
      ),
    ),

    italic: $ => prec.right(0,
      seq(
        alias($.italic_open, "open"),
        repeat1(
          choice(
            $.word,
            $.bold,
            $.underline,
            $.strikethrough,
            $.verbatim,
          ),
        ),
        alias($.italic_close, "close")
      ),
    ),

    underline: $ => prec.right(0,
      seq(
        alias($.underline_open, "open"),
        repeat1(
          choice(
            $.word,
            $.bold,
            $.italic,
            $.strikethrough,
            $.verbatim,
          ),
        ),
        alias($.underline_close, "close")
      ),
    ),

    strikethrough: $ => prec.right(0,
      seq(
        alias($.strikethrough_open, "open"),
        repeat1(
          choice(
            $.word,
            $.bold,
            $.italic,
            $.underline,
            $.verbatim,
          ),
        ),
        alias($.strikethrough_close, "close")
      ),
    ),

    verbatim: $ => prec.right(0,
      seq(
        alias($.verbatim_open, "open"),
        repeat1($.word),
        alias($.verbatim_close, "close")
      ),
    ),

    inline_math: $ => prec.right(0,
      seq(
        alias($.inline_math_open, "open"),
        repeat1($.word),
        alias($.inline_math_close, "close")
      ),
    ),

    // word: _ => choice(/\p{L}+/, /\p{N}+/),
  }
});

// function gen_markup($, kind) {
//   return prec.right(0,
//     seq(
//       // alias($[kind + "_open"], $.open),
//       // alias(repeat1($.word), $.text),
//       // alias($[kind + "_close"], $.close),
//       $[kind + "_open"],
//       repeat1($.word),
//       $[kind + "_close"]
//     ),
//   );
// }

// vim: ts=2 sts=2 sw=2
