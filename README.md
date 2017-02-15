# Подсчёт количества машин (тестовый пример)
Подсчёт машин на видео с камер видеонаблюдения.
Реализован только дневной режим. Алгоритм **не использует** нейросети.
Алгоритм для большинства кадров из тестового видео работает быстрее Realtime. Ограничение на `24/1001fps` задано [здесь](https://github.com/slavanap/CarCounterTest/blob/master/QtUtility.cpp#L78).
Для отображения результатов промежуточных шагов алгоритма можно переключить `#define SHOW_STEPS` в `1` в [этом файле](https://github.com/slavanap/CarCounterTest/blob/master/processing.cpp).

Алгоритм был выбран таким, как самое быстрое и универсальное решение с минимальным числом параметров для адаптации к другим входным данным (без использования машинного обучения, realtime время работы). Возможна дальнейшая оптимизация времени работы алгоритма, однако задание этого не требует. Для ночного режима не удалось подобрать универсальные критерии для нахождения контуров пары фар машин на кадре без использования машинного обучения.


## Как пользоваться примером
1. `File->Open File...`, выбрать видеофайл. После этого начнёт проигрываться видео.
2. С помощью красных точек (вверху слева) задать положения ломанной, пересекая которую будет производиться учёт машины.
3. Точки можно перемещать левой кнопкой мыши. По правой кнопке доступно меню для добавления/удаления точек (добавление - при щелчке по отрезку, удаление - при щелчке по точке). По умолчанию направление движения транспортных средств считается но нормали вниз. Для его изменения (вверх) доступен пункт меню, который присутствует в контекстном меню для точки и применяется для отрезка от выбранной точки до следующей.
4. Отслеживаемые машины обводятся синим прямоугольником.
5. Колличество учтённых машин для отрезка отображается в середине этого отрезка. При пересечении машиной заданного отрезка, отрезок загоряется зелёным цветом.

## Общее описание алгоритма
Алгоритм подсчёта машин реализован в файле [processing.cpp](https://github.com/slavanap/CarCounterTest/blob/master/processing.cpp#L146-L233).

Шаги алгоритма:

1. Вычисление яркости для предыдущего и текущего кадров, их размытие по гауссу с радиусом 5 пикселей, вычисление модуля разности между полученными кадрами (`cv::cvtColor`, `cv::GaussianBlur`, `cv::absdiff`)
2. Вычисление бинарной маски для движущихся объектов (`cv::threshold`)
3. Приминение морфологии: `РРСРРСРРС`, где Р - мат. расширение (`cv::dilate`), C - мат. сужение (`cv::erode`)
4. Поиск контуров замкнутых областей (`cv::convexHull`), инициализания
[CarDescriptor](https://github.com/slavanap/CarCounterTest/blob/master/processing.h#L15-L30)
для отслеживания их перемещения
5. Сопоставление объектов, найденных на предыдущем шаге, с объектами, найденными на текущем шаге ([matchCars](https://github.com/slavanap/CarCounterTest/blob/master/processing.cpp#L80-L112)). Состоит из:
  * предсказания следующей позиции объекта по макс. 5 точкам из истории перемещения ([predictNextPosition](https://github.com/slavanap/CarCounterTest/blob/master/processing.cpp#L31-L48)),
  * поиска объекта в радиусе `sqrt(w^2 + h^2)` относительно предсказанной точки,
  * удаления объектов из списка отслеживаемых после их отсутствия в течение 5 кадров.
