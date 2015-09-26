#!/usr/bin/env python3
import os
from time import sleep
from fcntl import fcntl, F_GETFL, F_SETFL
from subprocess import call, Popen, PIPE

KEY_LINE_BEGIN  = '\x01'
KEY_LINE_END    = '\x05'
KEY_INPUT_END   = '\x04'
KEY_TAB         = '\t'
KEY_NEWLINE     = '\n'
KEY_ESC         = '\x1b'
KEY_BACKSPACE   = '\x7f'

KEY_MOD_INT     = '\x5b'
KEY_MOD_END     = '\x7e'

KEY_HOME        = KEY_ESC + KEY_MOD_INT + '\x31' + KEY_MOD_END
KEY_INSERT      = KEY_ESC + KEY_MOD_INT + '\x32' + KEY_MOD_END
KEY_DELETE      = KEY_ESC + KEY_MOD_INT + '\x33' + KEY_MOD_END
KEY_END         = KEY_ESC + KEY_MOD_INT + '\x34' + KEY_MOD_END
KEY_PAGEUP      = KEY_ESC + KEY_MOD_INT + '\x35' + KEY_MOD_END
KEY_PAGEDOWN    = KEY_ESC + KEY_MOD_INT + '\x36' + KEY_MOD_END
KEY_ARROW_UP    = KEY_ESC + KEY_MOD_INT + 'A'
KEY_ARROW_DOWN  = KEY_ESC + KEY_MOD_INT + 'B'
KEY_ARROW_RIGHT = KEY_ESC + KEY_MOD_INT + 'C'
KEY_ARROW_LEFT  = KEY_ESC + KEY_MOD_INT + 'D'

KEY_NULL        = '\0'

class CursorOverflowException(Exception):
    def __init__(self, idxCurr, idxDest):
        super().__init__('Cannot move cursor from {} to {}.'
            .format(idxCurr, idxDest))
        self.currentIndex = idxCurr
        self.destIndex = idxDest

class AssertionException(AssertionError):
    def __init__(self, message, expected, found):
        super().__init__('Assert failed - {}: expected [{}], found [{}].'
            .format(message, expected, found))
        self.message = message
        self.expected = expected
        self.found = found

class CmdReaderTester():
    def __init__(self, argv=None, buffer_size=1000):
        ''' init a tester with specified buffer_size. '''
        self._buffer = [None] * buffer_size
        # indicating the end of the buffer
        self._buffer[0] = KEY_NULL
        # the cursor's position used in simulation
        self.cursor_position = 0
        # counting the bells
        self.bell_count = 0
        # store the history so far
        self.history = []

        # private members
        self._proc = None
        self._seq = b''
        self._seq_bol_pos = 0

        # create the subprocess
        if argv is not None:
            self._open(argv)

    @property
    def length(self):
        ''' returns the length of the buffer. '''
        return self._buffer.index(KEY_NULL)

    @property
    def raw(self):
        ''' returns a copy of the raw sequences at the last line. '''
        return self._seq[_seq_bol_pos:]


    def buffer(self, rtrim=False):
        ''' returns the string content of the buffer.
            trims spaces after the cursor if rtrim is set to True. '''
        if rtrim:
            pos = self.length
            while pos > 0 and self._buffer[pos - 1] == b' ' and self.cursor_position < (pos - 1):
                pos -= 1
        else:
            pos = self.length
        return b''.join(self._buffer[:pos]).decode('utf8')

    def input(self, seq):
        ''' pipe the keyboard sequence to stdin.
            also, simulate the response from stdout behind the scene. '''

        # convert to bytes before writing
        if isinstance(seq, str):
            seq = seq.encode()

        # feed it and flush it
        self._proc.stdin.write(seq)
        self._proc.stdin.flush()

        # wait for a short while to let the program process stdout
        # data may be dropped if the time is too short!
        sleep(.01)

        # shell simulation
        out = self._proc.stdout.read()
        if out is not None:
            for b in out:
                char = bytes((b,))
                self._seq += char
                self._inputChar(char)

    def check(self, buf=None, cur_pos=None, bell_count=None, reset_counter=True, strict=True):
        ''' check specified parameters are correct.
            an error with related variables is raised if the assert fails.
            reset bell counter after checking if reset_counter is True (default). '''

        cls = self.__class__
        bufeqfn = cls._check_equal if strict else cls._check_rstrip

        if buf is not None:
            bufeqfn         (buf,        self.buffer(True),    'buffer mismatch'         )
        if cur_pos is not None:
            cls._check_equal(cur_pos,    self.cursor_position, 'cursor position mismatch')
        if bell_count is not None:
            # ensure bell count would be cleared anyway
            tmp_bell_count = self.bell_count
            if reset_counter:
                self.bell_count = 0
            cls._check_equal(bell_count, tmp_bell_count,       'bell count mismatch'     )

    def test(self, seq, buf, cur_pos, bell_count=0, strict=True):
        ''' a shortand method to check all the parameters.
            useful when making atomic tests. '''
        self.input(seq)
        return self.check(buf, cur_pos, bell_count, strict=strict)

    def is_ended(self):
        ''' check if the program was exited normally. '''
        rcode = self._proc.poll()
        return rcode is not None

    def end(self):
        ''' terminate the process. '''
        self._proc.terminate()

    def _open(self, argv):
        ''' create a subprocess, piping its stdin and stdout. '''
        _p = self._proc = Popen(argv, stdin=PIPE, stdout=PIPE)
        fcntl(_p.stdout, F_SETFL, fcntl(_p.stdout, F_GETFL) | os.O_NONBLOCK)

    def _inputChar(self, char):
        ''' stdout response handler. '''
        if char == b'\a':
            # count bell, do not move cursor
            self.bell_count += 1
        elif char == b'\b':
            # move cursor back if possible
            if self.cursor_position == 0:
                raise CursorOverflowException(0, -1)
            self.cursor_position -= 1
        elif char == b'\n':
            # push current line to history and start a new line
            self.history.append(self.buffer())
            self._seq_bol_pos = self.length + 1
            self._buffer[0] = KEY_NULL
            self.cursor_position = 0
        else:
            is_last_char = self._buffer[self.cursor_position] == KEY_NULL
            # overflow detection
            if self.cursor_position > len(self._buffer) - 2:
                raise CursorOverflowException(self.cursor_position, self.cursor_position + 1)
            # printable char; do so as in replace mode
            self._buffer[self.cursor_position] = char
            # expand the buffer if inserting at last
            if is_last_char:
                self._buffer[self.cursor_position + 1] = KEY_NULL
            self.cursor_position += 1

    @staticmethod
    def _check_equal(answer, value, assert_errmsg):
        ''' internal checking for equality. '''
        if answer != value:
            raise AssertionException(assert_errmsg, answer, value)

    @staticmethod
    def _check_rstrip(answer, value, assert_errmsg):
        ''' internal checking ingoring spaces at the right. '''
        if answer.rstrip() != value.rstrip():
            raise AssertionException(assert_errmsg, answer.rstrip(), value.rstrip())

class PrintHelper():
    key_disp = {}
    key_disp[' '            ] = '␣'
    key_disp[KEY_LINE_BEGIN ] = '↖'
    key_disp[KEY_LINE_END   ] = '↘'
    key_disp[KEY_INPUT_END  ] = '◼'
    key_disp[KEY_TAB        ] = '↹'
    key_disp[KEY_NEWLINE    ] = '↲'
    key_disp[KEY_BACKSPACE  ] = '⌫'
    key_disp[KEY_INSERT     ] = '⎀'
    key_disp[KEY_HOME       ] = '⇤'
    key_disp[KEY_DELETE     ] = '⌦'
    key_disp[KEY_END        ] = '⇥'
    key_disp[KEY_PAGEUP     ] = '⇞'
    key_disp[KEY_PAGEDOWN   ] = '⇟'
    key_disp[KEY_ARROW_UP   ] = '↑'
    key_disp[KEY_ARROW_DOWN ] = '↓'
    key_disp[KEY_ARROW_RIGHT] = '→'
    key_disp[KEY_ARROW_LEFT ] = '←'

    @classmethod
    def disp(cls, ctrl_str):
        ''' print each key in human-readable format.
            pad each entry in 2 characters. '''
        buf = ''
        rtn = ''
        escmode = False
        for char in ctrl_str:
            if ord(char) < 32 or ord(char) == 127 or escmode or char == ' ':
                if char == KEY_ESC:
                    escmode = True
                # we also handle space in key_disp
                buf += char
                mkey = cls.key_disp.get(buf, None)
                if mkey is not None:
                    rtn += mkey + ' '
                    buf = ''
                    escmode = False
            else:
                # printable char
                rtn += str(char) + ' '
        if buf:
            raise BaseException('bad key sequence: {}'.format(buf.encode()))
        return rtn

    @staticmethod
    def highlight(s, n):
        ''' highlight the n-th characters in s '''
        if n > len(s):
            raise BaseException('cannot highlight the {} word of the text of length {}'.format(n, len(s)))
        elif n == len(s):
            # append the cursor at last, with a distinguishable symbol %
            return s + '\033[07m%\033[0m'
        return ''.join([s[:n], '\033[07m', s[n], '\033[0m', s[n+1:]])

def make(target=None):
    if target is None:
        call('make')
    else:
        call(['make', target])

def test_key_mapping():
    make('test')
    print('*** Please ensure that the following key identifiers are correct. ***')
    p = CmdReaderTester(['./testAsc'])
    p.input(''.join([
        'aA1~#$',        # LITERAL
        KEY_LINE_BEGIN,  KEY_LINE_END,
        KEY_TAB,         KEY_NEWLINE,
        KEY_BACKSPACE,   KEY_HOME,
        KEY_END,         KEY_DELETE,
        KEY_PAGEUP,      KEY_PAGEDOWN,
        KEY_ARROW_UP,    KEY_ARROW_DOWN,
        KEY_ARROW_RIGHT, KEY_ARROW_LEFT,
        KEY_INPUT_END    # end of input
    ]))
    print('\n'.join(p.history))
    p.end()

def test_official_suite(fname='./cmdReader'):
    p = CmdReaderTester([fname])

    _SEQ_LONG_HISTORY = ''.join(
        [ str(i) + KEY_NEWLINE
            for i
            in (2, 3, 4, 5, 6, 7, 8, 9, 0, 11, 12, 13, 14) ]
    )

    testSuite = [
        ('Hello World'       , 'Hello World'                      , 11) ,
        ('  '                , 'Hello World  '                    , 13) ,
        (KEY_LINE_BEGIN      , 'Hello World  '                    , 0)  ,
        (' '                 , ' Hello World  '                   , 1)  ,
        ('YaYa'              , ' YaYaHello World  '               , 5)  ,
        (' '                 , ' YaYa Hello World  '              , 6)  ,
        (KEY_DELETE * 2      , ' YaYa llo World  '                , 6)  ,
        (KEY_BACKSPACE * 3   , ' Yallo World  '                   , 3)  ,
        (KEY_NEWLINE * 2     , ''                                 , 0)  ,
        (KEY_ARROW_UP        , 'Yallo World'                      , 11) ,
        (KEY_ARROW_LEFT * 6  , 'Yallo World'                      , 5)  ,
        ('w'                 , 'Yallow World'                     , 6)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_ARROW_UP * 3    , 'Yallo World'                      , 11  , 1) ,
        ('!!'                , 'Yallo World!!'                    , 13) ,
        (KEY_ARROW_DOWN * 4  , ''                                 , 0   , 2) ,
        ('You may say'       , 'You may say'                      , 11) ,
        (KEY_HOME            , 'You may say'                      , 0)  ,
        ('  '                , '  You may say'                    , 2)  ,
        (KEY_ARROW_UP * 2    , 'Yallo World'                      , 11) ,
        (KEY_BACKSPACE * 9   , 'Ya'                               , 2)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        # TODO: atomic operation to check history
        ('I am a dreamer'    , 'I am a dreamer'                   , 14) ,
        ('  '                , 'I am a dreamer  '                 , 16) ,
        (KEY_ARROW_UP        , 'Ya'                               , 2)  ,
        ('Da'                , 'YaDa'                             , 4)  ,
        (KEY_ARROW_DOWN      , 'I am a dreamer  '                 , 16) ,
        (KEY_ARROW_UP        , 'Ya'                               , 2)  ,
        (KEY_BACKSPACE * 3   , ''                                 , 0   , 1) ,
        (KEY_DELETE          , ''                                 , 0   , 1) ,
        ('   '               , '   '                              , 3)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_ARROW_UP        , 'Ya'                               , 2)  ,
        (KEY_ARROW_UP        , 'Yallow World'                     , 12) ,
        (KEY_ARROW_DOWN * 3  , ''                                 , 0   , 1) ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        ('But not'           , 'But not'                          , 7)  ,
        ('  '                , 'But not  '                        , 9)  ,
        (KEY_HOME            , 'But not  '                        , 0)  ,
        (KEY_END             , 'But not  '                        , 9)  ,
        (KEY_ARROW_LEFT * 5  , 'But not  '                        , 4)  ,
        ('I\'m'              , 'But I\'mnot  '                    , 7)  ,
        (' '                 , 'But I\'m not  '                   , 8)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_ARROW_UP        , 'But I\'m not'                     , 11) ,
        (' '                 , 'But I\'m not '                    , 12) ,
        ('the only one.'     , 'But I\'m not the only one.'       , 25) ,
        (KEY_ARROW_UP * 2    , 'Yallow World'                     , 12) ,
        (KEY_DELETE * 2      , 'Yallow World'                     , 12  , 2) ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_ARROW_UP        , 'Yallow World'                     , 12) ,
        (KEY_ARROW_UP        , 'But I\'m not'                     , 11) ,
        (KEY_ARROW_UP        , 'Ya'                               , 2)  ,
        ('...'               , 'Ya...'                            , 5)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        # TODO: atomic operation to check history
        ('I hope someday'    , 'I hope someday'                   , 14) ,
        (KEY_ARROW_LEFT * 8  , 'I hope someday'                   , 6)  ,
        (KEY_TAB             , 'I hope   someday'                 , 8)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_TAB             , ' ' * 8                            , 8)  ,
        ('you\'ll join us.'  , '        you\'ll join us.'         , 23) ,
        (KEY_HOME            , '        you\'ll join us.'         , 0)  ,
        ('1'                 , '1        you\'ll join us.'        , 1)  ,
        (KEY_LINE_END        , '1        you\'ll join us.'        , 24) ,
        (KEY_TAB             , '1        you\'ll join us.       ' , 32) ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (_SEQ_LONG_HISTORY   , ''                                 , 0)  ,
        ('And the world'     , 'And the world'                    , 13) ,
        (KEY_PAGEUP          , '5'                                , 1)  ,
        (KEY_PAGEUP          , 'Yallow World'                     , 12) ,
        (KEY_PAGEUP          , 'Yallo World'                      , 11) ,
        (KEY_PAGEUP          , 'Yallo World'                      , 11  , 1) ,
        (KEY_PAGEDOWN        , '4'                                , 1)  ,
        (KEY_PAGEDOWN        , '14'                               , 2)  ,
        (KEY_PAGEDOWN        , 'And the world'                    , 13) ,
        (KEY_PAGEDOWN        , 'And the world'                    , 13  , 1) ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        ('will live as one!' , 'will live as one!'                , 17) ,
        (KEY_PAGEUP          , '6'                                , 1)  ,
        (' '                 , '6 '                               , 2)  ,
        ('imagine'           , '6 imagine'                        , 9)  ,
        (KEY_NEWLINE         , ''                                 , 0)  ,
        (KEY_PAGEDOWN        , ''                                 , 0   , 1) ,
        (KEY_ARROW_UP * 12   , '5'                                , 1)  ,
        (KEY_PAGEDOWN * 2    , ''                                 , 0)  ,
        # end of the test
    ]
    cnt = 0
    cnt_success = 0
    for test in testSuite:
        cnt += 1
        try:
            # ATTENTION: disabled a test
            # p.test(test[0], None, test[2] + 5, (test[3] if len(test) > 3 else 0))

            # ATTENTION: output is compare against trimmed str
            p.test(
                test[0],
                'cmd> ' + test[1],
                test[2] + 5,
                (test[3] if len(test) > 3 else 0),
                False  # strict
            )
            cnt_success += 1
            print('{:4}: \033[92mSUCCESS\033[0m - {:32} - [{}]'.format(
                cnt,
                PrintHelper.disp(test[0]),
                PrintHelper.highlight(p.buffer(), p.cursor_position)
            ))
        except AssertionException as e:
            print('{:4}: \033[91mFAILURE\033[0m - {:32} - [{}]'.format(
                cnt,
                PrintHelper.disp(test[0]),
                PrintHelper.highlight(p.buffer(), p.cursor_position)
            ))
            print(e)

    if cnt_success == cnt:
        judge = '\033[92m\033[01mAll tests passed!\033[0m'
    else:
        judge = '\033[91m\033[01mSome Tests failed.\033[0m'
    print('*** {} *** Test finished with {} success and {} failure.'.format(
        judge, cnt_success, cnt - cnt_success
    ))

    # finalize
    p.input(KEY_INPUT_END)
    if not p.is_ended():
        raise BaseException('The program does not end properly.')

def cross_testing():
    from random import choice

    def _do_check():
        try:
            p2.check(
                buf=p1.buffer(True),
                cur_pos=p1.cursor_position,
                bell_count=p1.bell_count
            )
            # manually clear bell count as we tested
            p1.bell_count = 0
            return True
        except AssertionException as e:
            p1.bell_count = 0
            return e

    seq_simple = [
        'O', 'Q', 'A', 'O', 'Q', 'A', 'O', 'Q', 'A',
        KEY_NEWLINE,         KEY_TAB,
        KEY_BACKSPACE,       KEY_HOME,
        KEY_ARROW_RIGHT * 3, KEY_ARROW_LEFT * 3,
        KEY_END
    ]

    seq_complete = [
        'A', '@', '*', 'W', 'u', 'Q', ' ',
        KEY_LINE_BEGIN,  KEY_LINE_END,
        KEY_TAB,         KEY_NEWLINE,
        KEY_BACKSPACE,   KEY_HOME,
        KEY_END,         KEY_DELETE,
        KEY_PAGEUP,      KEY_PAGEDOWN,
        KEY_ARROW_UP,    KEY_ARROW_DOWN,
        KEY_ARROW_RIGHT, KEY_ARROW_LEFT
    ]

    def _batch(func, ops):
        for cnt in range(ops):
            c = func()
            p1.input(c)
            p2.input(c)
            result = _do_check()
            if result == True:
                print(('{:4}: \033[92mSUCCESS\033[0m - {:12}  p1: [{}]\n' + ' ' * 30 + 'p2: [{}]').format(
                    cnt + 1,
                    PrintHelper.disp(c),
                    PrintHelper.highlight(p1.buffer(), p1.cursor_position),
                    PrintHelper.highlight(p2.buffer(), p2.cursor_position)
                ))
            else:
                print(('{:4}: \033[91mFAILURE\033[0m - {:12}  p1: [{}]\n' + ' ' * 30 + 'p2: [{}]').format(
                    cnt + 1,
                    PrintHelper.disp(c),
                    PrintHelper.highlight(p1.buffer(), p1.cursor_position),
                    PrintHelper.highlight(p2.buffer(), p2.cursor_position)
                ))
                print(result)
                break

    p1 = CmdReaderTester(['./cmdReader-ref'], 65536)
    p2 = CmdReaderTester(['./cmdReader'],     65536)

    _batch(lambda: choice(seq_simple),   200)
    _batch(lambda: choice(seq_complete), int(1e8))

def main():
    tasks = [
        ('testing key mapping', test_key_mapping),
        ('making REF',          lambda: make('ref')),
        # ('testing REF',         lambda: test_official_suite('./cmdReader-ref')),
        ('making test HW2',     lambda: make()),
        # ('testing HW2',         lambda: test_official_suite('./cmdReader')),
        ('cross testing',       lambda: cross_testing()),
    ]

    for desc, func in tasks:
        print('========== {:^24} =========='.format(desc))
        func()
        print('')


if __name__ == '__main__':
    main()
