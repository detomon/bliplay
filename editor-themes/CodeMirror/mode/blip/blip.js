(function(mod) {
	if (typeof exports == "object" && typeof module == "object") // CommonJS
		mod(require("../../lib/codemirror"));
	else if (typeof define == "function" && define.amd) // AMD
		define(["../../lib/codemirror"], mod);
	else // Plain browser env
		mod(CodeMirror);
})(function(CodeMirror) {
"use strict";

CodeMirror.defineMode('blip', function() {

	var words = {};
	function define(style, string) {
		var split = string.split(' ');
		for(var i = 0; i < split.length; i++) {
			words[split[i]] = style;
		}
	};

	define('keyword', 'a adsr anv as at d data dc dcnv dn dr ds e g grp harm ' +
		'i instr load m mt no noi noise octave p pal pk pnv pr ps pt pulsekernel pw r ' +
		'rep rt s sam samp sample saw sawtooth si sin sinc sine sm sq sqr ' +
		'square st stepticks stt sw t tickrate tr track tri triangle tstepticks ' +
		'v vb vm vnv vs w wave x xb z');

	// Notes
  	var punc = "[]:;";
	var notes =['a', 'a#', 'b', 'c', 'c#', 'd', 'd#', 'e', 'f', 'f#', 'g', 'g#', 'h'];

	function tokenBase(stream, state) {
		if (stream.eatSpace()) return null;

		var ch = stream.next();

		if (punc.indexOf(ch) > -1) {
			return "punctuation";
		}
		if (ch === '\\') {
			stream.next();
			return null;
		}
		if (ch === '\'' || ch === '"') {
			state.tokens.unshift(tokenString(ch, "string"));
			return tokenize(stream, state);
		}
		if (ch === '%') {
			stream.skipToEnd();
			return 'comment';
		}
		if (notes.indexOf(ch) > -1) {
			var next = stream.peek();

			if (/[#\d]/.test(next)) {
				if (notes.indexOf(ch + '#') > -1) {
					stream.eat('#');
				}
				stream.eatWhile(/\d/);
				return 'attribute';
			}
		}
		if (/[-+]/.test(ch)) {
			if (/\d/.test(stream.peek())) {
				stream.eat(/[-+]/);
				stream.eatWhile(/\d/);
				return 'number';
			}
		}
		if (/\d/.test(ch)) {
			stream.eatWhile(/\d/);
			if (stream.peek() === '/') {
				stream.eat('/');
				stream.eatWhile(/\d/);
			}
			return 'number';
		}
		stream.eatWhile(/[\w-]/);
		var cur = stream.current();
		return words.hasOwnProperty(cur) ? words[cur] : null;
	}

	function tokenString(quote, style) {
		var close = quote;
		return function(stream, state) {
			var next, end = false, escaped = false;
			while ((next = stream.next()) != null) {
				if (next === close && !escaped) {
					end = true;
					break;
				}
				if (!escaped && next === quote && quote !== close) {
					state.tokens.unshift(tokenString(quote, style))
					return tokenize(stream, state)
				}
				escaped = !escaped && next === '\\';
			}
			if (end) state.tokens.shift();
			return style;
		};
	};

	function tokenize(stream, state) {
		return (state.tokens[0] || tokenBase) (stream, state);
	};

	return {
		startState: function() {return {tokens:[]};},
		token: function(stream, state) {
			return tokenize(stream, state);
		},
		closeBrackets: "[]''\"\"",
		lineComment: '%',
		fold: "brace"
	};
});

CodeMirror.defineMIME('text/x-blip', 'blip');

});
