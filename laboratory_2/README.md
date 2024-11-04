# Лабораторная работа №2. Использование библиотек

Текущий статус тестирования GitHub Actions: [![CI/CD](../../actions/workflows/classroom.yml/badge.svg?branch=main&event=workflow_dispatch)](../../actions/workflows/classroom.yml).

> [!Note]
> Чтобы GitHub Workflow отработал верно, файл с [функцией `main`](https://en.cppreference.com/w/c/language/main_function) должен называться `main.c`.
>
> [**Не забудьте инициализировать репозиторий!**](#инициализация-репозитория)

## Инициализация репозитория

После *первого клонирования* репозитория его необходимо инициализировать. Для этого Вам необходимо запустить соответствующий скрипт, для запуска требуется [Bash](https://en.wikipedia.org/wiki/Bash_(Unix_shell)) (есть в UNIX-подобных системах, в Windows – подойдёт [Git Bash](https://git-scm.com/)) и [GitHub CLI](https://cli.github.com/):

```bash
bash init-repo.sh
```

Скрипт необходимо запустить **только один раз**.

## Локальное тестирование

Вам предоставляется возможность запуска базовых тестов локальным способом. Для этого нужно:

1. Установить [Python](https://www.python.org/).
2. Убедиться, что у Вас установлены следующие библиотеки: `hashlib`, `difflib`, `pyperclip` (в ином случае, установить через [`pip`](https://pypi.org/project/pip/)).
3. Склонировать репозиторий рекурсивно `git clone <repo_url> --recursive`. В противном случае `submodule` тестов не склонируется и счастье не наступит.
4. В корне репозитория запустить `python tests.py <path/to/executable> <profile>`, где `profile` - это обобщенное название конфигурации сборки (по типу `CMAKE_BUILD_TYPE`) для настройки максимального времени исполнения тестов: *Release* (оптимизация), *Debug* (для отладки), далее идут комбинации *Debug* с санитайзером [*AddressSanitized*](https://clang.llvm.org/docs/AddressSanitizer.html), [*LeakSanitized*](https://clang.llvm.org/docs/LeakSanitizer.html), [*UndefinedBehaviorSanitized*](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html), [*ThreadSanitized*](https://clang.llvm.org/docs/ThreadSanitizer.html), [*MemorySanitized*](https://clang.llvm.org/docs/MemorySanitizer.html).
5. Посмотреть логи тестирования.

По умолчанию, запускаются все *категориальные* тесты. Если нужны конкретные категории, в аргументы запуска необходимо подать наименования категорий (см. ниже). Внимание: если тест требует две категории и был подан только один из них, тест *не будет запущен*, необходимо подать обе категории.

Список категорий:

* **`positive_test`** - корректный тест
* **`negative_test`** - некорректный тест
* **`positive_delta`** - значение $\delta$ больше нуля
* **`negative_delta`** - значение $\delta$ меньше нуля

Пример запуска:

```bash
python tests.py main.exe Release positive_test negative_delta
# запуск корректных тестов с выходным значением delta < 0,
# с макс. временем исполнением Release
```

Если результат расходится с ожидаемым, то будет выведена ошибка в формате:

```bash
====> ERROR: assert at extracted positional value [X] => [actual = Y] vs [expected = Z].
```

где `X` – позиция несовпадающего значения, `Y` – полученное вашей программой значение, `Z` – ожидаемое.

`X` может принимать значения 0, 1 и 2:

* 0 – "delta"
* 1 – "sample rate"
* 2 – "delta time"

```bash
====> ERROR: assert at extracted positional value [0] => [actual = 356554] vs [expected = 356544].
```
