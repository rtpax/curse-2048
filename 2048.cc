#include <ncurses/curses.h>
#include <thread>
#include <vector>
#include <chrono>
#include <random>

struct rc {
    int row, col;
};

struct add_prob {
    int value;
    int probability;
};

class grid {
private:
    static std::mt19937 rng;
    static grid* active_window;
    static bool window_started;
    static void on_interrupt(int arg) {
        endwin();
        exit(arg);
    }
    static void on_resize(int) {
        endwin();
        refresh();
        clear();
        if(active_window != nullptr)
            active_window->draw(true);
    }
    int ** _cells;
    int _dim;
    std::vector<add_prob> _probs;
    int _start_cells;
    rc get_random_empty_cell() {
        std::vector<rc> empties;
        for(int row = 0; row < _dim; ++row) {
            for(int col = 0; col < _dim; ++col) {
                if(_cells[row][col] == 0) {
                    empties.push_back(rc{row,col});
                }
            }
        }
        if(empties.empty()) {
            throw std::runtime_error("no empty cells");
        }
        std::uniform_int_distribution<int> dist(0, empties.size() - 1);
        return(empties[dist(rng)]);
    }
    rc insert_random_cell() {
        rc cell_to_add = get_random_empty_cell();
        int sum = 0;
        for(int i = 0; i < _probs.size(); ++i) {
            sum += _probs[i].probability;
        }
        std::uniform_int_distribution<int> dist(1, sum);
        int value_to_add = 0;
        int result = dist(rng);
        for(int i = 0; i < _probs.size(); ++i) {
            result -= _probs[i].probability;
            if(result <= 0) {
                value_to_add = _probs[i].value;
                break;
            }
        }
        if(value_to_add == 0)
            throw std::logic_error("failed to pick a value to add");
        _cells[cell_to_add.row][cell_to_add.col] = value_to_add;
        return cell_to_add;
    }
    void loop() {
        for(;;) {
            int ch = getch();
            if(ch == ERR) {
                std::this_thread::yield();
            } else {
                switch(ch) {
                case 'q':
                case 'Q':
                    return;
                case 'r':
                case 'R':
                    reset();
                    draw(false);
                    break;
                case KEY_UP:
                case 'w':
                case 'W':
                    if(up())
                        try{insert_random_cell();}catch(std::runtime_error&){}
                    draw(false);
                    break;
                case KEY_RIGHT:
                case 'd':
                case 'D':
                    if(right())
                        try{insert_random_cell();}catch(std::runtime_error&){}
                    draw(false);
                    break;
                case KEY_LEFT:
                case 'a':
                case 'A':
                    if(left())
                        try{insert_random_cell();}catch(std::runtime_error&){}
                    draw(false);
                    break;
                case KEY_DOWN:
                case 's':
                case 'S':
                    if(down())
                        try{insert_random_cell();}catch(std::runtime_error&){}
                    draw(false);
                    break;
                default:
                    break;
                }
            }
        }
    }
    void draw(bool redraw) {
        int maxx = getmaxx(stdscr);
        int maxy = getmaxy(stdscr);

        if(maxx < _dim * 7 || maxy < _dim * 4) {
            clear();
            mvaddstr(0, 0, "Window too small to display grid");
            return;
        }
        int basex = (maxx - (_dim * 7 + 1)) / 2;
        int basey = (maxy - (_dim * 4 + 1)) / 2;
        if(redraw) {
            clear();
            for(int row = 0; row < _dim; ++row) {
                for(int col = 0; col < _dim; ++col) {
                    int cornery = basey + row * 4;
                    int cornerx = basex + col * 7;
                    mvaddstr(cornery,cornerx," ------");
                    for(int i = 1; i <= 3; ++i)
                        mvaddch(cornery+i,cornerx,'|');
                }
            }
            for(int row = 0; row < _dim; ++row) {
                int cornery = basey + row * 4;
                int cornerx = basex + _dim * 7;
                for(int i = 1; i <= 3; ++i)
                    mvaddch(cornery+i,cornerx,'|');
            }
            for(int col = 0; col < _dim; ++col) {
                int cornery = basey + _dim * 4;
                int cornerx = basex + col * 7;
                mvaddstr(cornery, cornerx, " ------");
            }
        }
        for(int row = 0; row < _dim; ++row) {
            for(int col = 0; col < _dim; ++col) {
                int cornery = basey + row * 4;
                int cornerx = basex + col * 7;
                std::string numstr = std::to_string(_cells[row][col]);
                mvaddstr(cornery + 2, cornerx + 1, "      ");
                if(_cells[row][col] != 0)
                    mvaddstr(cornery + 2, cornerx + 4 - numstr.size() / 2, numstr.c_str());
            }
        }
        wmove(stdscr,maxy,maxx);
    }
public:
    grid(int dim, int start_cells, std::vector<add_prob> probs) {
        _dim = dim;
        _cells = new int*[_dim];
        for(int i = 0; i < _dim; ++i) {
            _cells[i] = new int[_dim];
        }
        _probs = probs;
        _start_cells = start_cells;
    }
    grid() : grid(4,2,{{2,9},{4,1}}) {}

    bool right() {
        bool moved = false;
        for(int row = 0; row < _dim; ++row) {
            int back = _dim - 1;
            while (back > 0) {
                bool no_move = true;
                for(int col = back - 1; col >= 0; --col) {
                    if(_cells[row][col] == _cells[row][col + 1] && _cells[row][col] != 0) {
                        _cells[row][col + 1] *= 2;
                        _cells[row][col] = 0;
                        back = col;
                        moved = true;
                        no_move = false;
                    } else if (_cells[row][col] != 0 && _cells[row][col + 1] == 0) {
                        _cells[row][col + 1] = _cells[row][col];
                        _cells[row][col] = 0;
                        moved = true;
                        no_move = false;
                    }
                }
                if(no_move)
                    break;
            }
        }
        return moved;
    }
    bool left() {
        bool moved = false;
        for(int row = 0; row < _dim; ++row) {
            int back = 0;
            while (back < _dim - 1) {
                bool no_move = true;
                for(int col = back + 1; col < _dim; ++col) {
                    if(_cells[row][col] == _cells[row][col - 1] && _cells[row][col] != 0) {
                        _cells[row][col - 1] *= 2;
                        _cells[row][col] = 0;
                        back = col;
                        moved = true;
                        no_move = false;
                    } else if (_cells[row][col] != 0 && _cells[row][col - 1] == 0) {
                        _cells[row][col - 1] = _cells[row][col];
                        _cells[row][col] = 0;
                        moved = true;
                        no_move = false;
                    }
                }
                if(no_move)
                    break;
            }
        }
        return moved;
    }
    bool down() {
        bool moved = false;
        for(int col = 0; col < _dim; ++col) {
            int back = _dim - 1;
            while (back > 0) {
                bool no_move = true;
                for(int row = back - 1; row >= 0; --row) {
                    if(_cells[row][col] == _cells[row + 1][col] && _cells[row][col] != 0) {
                        _cells[row + 1][col] *= 2;
                        _cells[row][col] = 0;
                        back = row;
                        moved = true;
                        no_move = false;
                    } else if (_cells[row][col] != 0 && _cells[row + 1][col] == 0) {
                        _cells[row + 1][col] = _cells[row][col];
                        _cells[row][col] = 0;
                        moved = true;
                        no_move = false;
                    }
                }
                if(no_move)
                    break;
            }
        }
        return moved;
    }
    bool up() {
        bool moved = false;
        for(int col = 0; col < _dim; ++col) {
            int back = 0;
            while (back < _dim - 1) {
                bool no_move = true;
                for(int row = back + 1; row < _dim; ++row) {
                    if(_cells[row][col] == _cells[row - 1][col] && _cells[row][col] != 0) {
                        _cells[row - 1][col] *= 2;
                        _cells[row][col] = 0;
                        back = row;
                        moved = true;
                        no_move = false;
                    } else if (_cells[row][col] != 0 && _cells[row - 1][col] == 0) {
                        _cells[row - 1][col] = _cells[row][col];
                        _cells[row][col] = 0;
                        moved = true;
                        no_move = false;
                    }
                }
                if(no_move)
                    break;
            }
        }
        return moved;
    }

    void open_window() {
        if(!window_started) {
            signal(SIGINT, &on_interrupt);
            signal(SIGWINCH, &on_resize);
            window_started = true;
        }

        if(active_window != nullptr) {
            throw std::runtime_error("only one window at a time");
        }
        active_window = this;
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, true);
        if(nodelay(stdscr, true) == ERR)
            throw std::runtime_error("nodelay error");

        draw(true);
        try {
            loop();
        } catch (std::runtime_error) {
            
        } catch (std::logic_error) {

        }
        endwin();
        active_window = nullptr;
    }

    int reset() {
        for(int i = 0; i < _dim; ++i) {
            for(int j = 0; j < _dim; ++j) {
                _cells[i][j] = 0;
            }
        }
        for(int i = 0; i < _start_cells; ++i) {
            insert_random_cell();
        }
    }
};

grid* grid::active_window = nullptr;
bool grid::window_started = 0;
std::mt19937 grid::rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

int main() {
    grid g;
    g.reset();
    g.open_window();
}




