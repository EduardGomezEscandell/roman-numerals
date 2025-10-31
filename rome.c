#include <stdbool.h>
#include <assert.h>

#include "rome.h"
#include "result.h"

/*
 * In order to parse (or reject) inputs, the following three steps are performed:
 *  1. The roman numeral is tokenized. A token is either a prefix-sufix pair (like IV in MMDIV), a repeated roman digit
 *     (like MM in the previous example), or a lonely digit (like the D in the previous example). Invalid tokens may be
 *     rejected here (e.g: LL or XM).
 *  2. Token ordering is strict in roman numberals, so any invalid sequence of tokens like IVIV (tokenized as IV,IV) is
 *     rejected here.
 *  3. The values of the tokens are added up.
 */

enum token_type {
    PAIR,     // Prefix-suffix pair  (IV, XC, etc.)
    REPEAT    // Repeated value (I, II, CCC, etc.). Single-numerals (I, L) are considered trivial repeats.
};

struct token {
    enum token_type type; // Type of token
    union {
        // When parsing a pair
        struct {
            int prefix;
            int suffix;
        };
        // When parsing repeated digits
        struct {
            int digit;
            int count;
        };
    };
};

// Converts a token into its numeral value (e.g. XC returns 90)
int token_value(struct token t);

// Converts one-character digits to their character.
// This means 1, 5, 10, etc. are valid inputs, but 2, 9, 600 are not.
char digit_to_roman(int d);

// Writes a token to a buffer of length 'len'. It returns true if the buffer was large enough to fit the string.
// 4 bytes is enough to fit any token.
bool sprint_token(char* buff, int len, struct token t);

// Checks that a prefix-suffix pair is valid: IV is good but LC is not.
bool valid_pair(int prefix, int suffix);

// Checks that a repetition is valid. III is good but LL is not.
bool valid_repeats(int main, int count);

// Checks that two tokens can go one after another. (C)(I) is good but (IX)(I) is not.
bool valid_sequence(struct token first, struct token second);

// Parses the numerical value of a character into *out. Returns false if the character is not a roman numeral.
bool parse_roman_character(char c, int *out);

// Reads from str and writes the resulting token to t.
// Returns a result containing the count of characters consumed, or an error message otherwise.
struct result consume_next_token(char const* str, struct token* t);

struct result parse_roman_number(const char* str) {
    if (*str == '\0') {
        return errorf("input is empty");
    }

    struct token prev;
    int tally = 0;

    // Get first token
    {
        const struct result res = consume_next_token(str, &prev);
        if (res.error) {
            return res;
        }
        str += res.value;
        tally += token_value(prev);
    }

    // Get remaining tokens
    while (*str != '\n' && *str != '\0') {
        struct token next;
        const struct result res = consume_next_token(str, &next);
        if (res.error) {
            return res;
        }
        assert(res.value > 0);

        if (!valid_sequence(prev, next)) {
            char buff1[32], buff2[32];
            sprint_token(buff1, 32, prev);
            sprint_token(buff2, 32, next);
            return errorf(" %s cannot be followed by %s", buff1, buff2);
        }

        str += res.value;
        tally += token_value(next);
        prev = next;
    }

    return success(tally);
}

struct result consume_next_token(char const* str, struct token* t) {
    // First character
    if (*str == '\0' || *str == '\n') {
        return errorf("EOF");
    }

    int first;
    if (!parse_roman_character(str[0], &first)) {
        return errorf("invalid character: %c", str[0]);
    }

    // Second character -> decides between pair and repeat
    if (str[1] == '\0' || str[1] == '\n') {
        *t = (struct token) {.type = REPEAT, .digit = first, .count = 1};
        return success(1);
    }

    int second;
    if (!parse_roman_character(str[1], &second)) {
        return errorf("invalid character: %c", str[0]);
    }

    if (first < second) {
        // It's a pair!
        // Something like XL or IV
        if (!valid_pair(first, second)) {
            return errorf("invalid pair: %c%c", str[0], str[1]);
        }
        *t = (struct token) {.type = PAIR, .prefix = first, .suffix = second};
        return success(2); // 2 characters consumed
    }

    if (first > second) {
        // It was a lonely character (trivial repeat)
        *t = (struct token) {.type = REPEAT, .digit = first, .count = 1};
        return success(1); // Only the first character was consumed, next invocation can deal with the second one
    }

    // Repetition! Keep reading until character changes
    const char* it;
    for (it = str+2; *str == *it; ++it) {
        if (!parse_roman_character(*str, &second)) {
            return errorf("invalid character: %c", *str);
        }
    }
    const int num_repeats = (int)(it - str);
    if (!valid_repeats(first, num_repeats)) {
        return errorf("character %c cannot appear %d times in a row", *str, num_repeats);
    }
    *t = (struct token) {.type = REPEAT, .digit = first, .count = num_repeats};
    return success(t->count);
}

int token_value(const struct token t) {
    switch (t.type) {
        case REPEAT:
            return t.digit * t.count;
        case PAIR:
            return t.suffix - t.prefix;
        default:
            assert(false);
    }
}

bool sprint_token(char *const buff, const int len, const struct token t) {
    switch (t.type) {
        case REPEAT: {
            if (len < t.count+1) {
                return false;
            }
            const char d = digit_to_roman(t.digit);
            for (int i = 0; i<t.count; ++i) {
                buff[i] = d;
            }
            buff[t.count] = '\0';
            return true;
        }
        case PAIR: {
            if (len < 3) {
                return false;
            }
            buff[0] = digit_to_roman(t.prefix);
            buff[1] = digit_to_roman(t.suffix);
            buff[2] = '\0';
            return true;
        }
        default:
            assert(false);
    }
}

bool valid_sequence(const struct token first, const struct token second) {
    /*
    I have determined that the following two rules are true:
        1. The first character of consecutive tokens must decrease (XXX can be followed by IX because X<I)
        2. If the first token is V, L or D; the following must also decrease in last character (V cannot be followed by IV because V=V)
    These rules exclude things like XX followed by X, because this would have been tokenized as XXX.
    They also exclude invalid tokens like VC, IM, LL, etc.

    See the rules expanded (A+ means any of A,AA,AAA):
        I+ is terminal
        IV is terminal
        IX is terminal
        V  can be followed by I+ (rule 2 disallows IV,IX)
        X+ can be followed by I+,IV,V,IX
        XL can be followed by I+,IV,V,IX
        XC can be followed by I+,IV,V,IX
        L  can be followed by I+,IV,V,IX,X+ (rule 2 disallows XL,XC)
        C+ can be followed by I+,IV,V,IX,X+,XL,L,XC
        CD can be followed by I+,IV,V,IX,X+,XL,L,XC
        CM can be followed by I+,IV,V,IX,X+,XL,L,XC
        D  can be followed by I+,IV,V,IX,X+,XL,L,XC,C+ (rule 2 disallows CD,CM)
        M+ can be followed by I+,IV,V,IX,X+,XL,L,XC,C+,CD,D,CM
    */

    const int first_prefix = first.type==PAIR ? first.prefix : first.digit;

    if (first_prefix == 5 || first_prefix == 50 || first_prefix == 500) {
        const int second_suffix = second.type==PAIR ? second.suffix : second.digit;
        return first_prefix > second_suffix;
    }

    const int second_prefix = second.type==PAIR ? second.prefix : second.digit;
    return first_prefix > second_prefix;
}

bool valid_pair(const int prefix, const int suffix) {
    switch (suffix) {
        case 5:
        case 10:
            return prefix == 1;
        case 50:
        case 100:
            return prefix == 10;
        case 500:
        case 1000:
            return prefix == 100;
        default:
            return false;
    }
}

bool valid_repeats(const int main, const int count) {
    if (count == 0) {
        return false;
    }

    switch (main) {
        case 5:
        case 50:
        case 500:
            return count==1;
        case 1:
        case 10:
        case 100:
            return count<4;
        case 1000:
            return true;
        default:
            return 0;
    }
}

bool parse_roman_character(const char c, int *const out) {
    switch (c) {
        case 'I':
            *out = 1;
            return true;
        case 'V':
            *out = 5;
            return true;
        case 'X':
            *out = 10;
            return true;
        case 'L':
            *out = 50;
            return true;
        case 'C':
            *out = 100;
            return true;
        case 'D':
            *out = 500;
            return true;
        case 'M':
            *out = 1000;
            return true;
        default:
            return false;
    }
}

char digit_to_roman(const int d) {
    switch (d) {
        case 1:
            return 'I';
        case 5:
            return 'V';
        case 10:
            return 'X';
        case 50:
            return 'L';
        case 100:
            return 'C';
        case 500:
            return 'D';
        case 1000:
            return 'M';
        default:
            return '?';
    }
}