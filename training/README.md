# Cell Analyzer - Neural Network Training

Этот модуль содержит скрипты для обучения U-Net модели для детекции клеток.

## Требования

```bash
pip install -r requirements.txt
```

### Основные зависимости:
- PyTorch >= 2.0
- segmentation-models-pytorch
- albumentations (аугментации)
- opencv-python
- numpy
- pillow
- tqdm
- tensorboard (для визуализации обучения)

## Структура проекта

```
training/
├── README.md                 # Этот файл
├── requirements.txt          # Зависимости Python
├── dataset.py               # Загрузка и подготовка данных
├── model.py                 # Архитектура U-Net
├── train.py                 # Скрипт обучения
├── export_onnx.py           # Экспорт в ONNX
├── cvat_utils.py            # Утилиты для работы с CVAT
├── augmentations.py         # Аугментации данных
├── utils.py                 # Вспомогательные функции
├── config.yaml              # Конфигурация обучения
└── data/
    ├── images/              # Исходные изображения
    ├── masks/               # Маски (разметка из CVAT)
    ├── train.txt            # Список файлов для обучения
    └── val.txt              # Список файлов для валидации
```

## Workflow: От разметки до обученной модели

### Шаг 1: Разметка данных в CVAT

#### 1.1 Создание проекта в CVAT

1. Установите CVAT локально или используйте https://app.cvat.ai
2. Создайте новый проект:
   - **Name**: Cell Detection Project
   - **Labels**:
     - `background` (class_id: 0)
     - `cell_type_a` (class_id: 1)
     - `cell_type_b` (class_id: 2)
     - `cell_type_c` (class_id: 3)
   - **Task type**: Instance Segmentation

#### 1.2 Загрузка изображений

```bash
# Скопируйте ваши изображения в папку
cp /path/to/your/images/*.jpg data/images/
```

Загрузите изображения в CVAT через веб-интерфейс.

#### 1.3 Разметка

Для каждого изображения:
1. Выберите инструмент "Polygon" или "Mask"
2. Обведите контуры клеток
3. Присвойте правильную метку (cell_type_a, cell_type_b, cell_type_c)
4. **Важно**: Разметьте ВСЕ клетки на изображении

#### 1.4 Экспорт разметки

Экспортируйте разметку в формате:
- **Format**: COCO 1.0 (рекомендуется) или Pascal VOC
- **Save images**: Yes

```bash
# После экспорта, распакуйте архив
unzip annotations.zip -d data/
```

#### 1.5 Конвертация аннотаций

```bash
# Конвертация COCO -> маски PNG
python cvat_utils.py --format coco --input data/annotations.json --output data/masks/

# Или Pascal VOC -> маски PNG
python cvat_utils.py --format pascal_voc --input data/Annotations/ --output data/masks/
```

### Шаг 2: Подготовка датасета

```bash
# Создание train/val split (80/20)
python dataset.py --prepare-split --data-dir data/ --train-ratio 0.8

# Проверка датасета
python dataset.py --visualize --num-samples 5
```

### Шаг 3: Настройка конфигурации

Отредактируйте `config.yaml`:

```yaml
# Training settings
model:
  encoder: resnet34          # resnet34, resnet50, efficientnet-b3
  encoder_weights: imagenet  # Предобученные веса
  num_classes: 4             # 1 (background) + 3 (типа клеток)
  input_size: 512            # 512 или 1024

training:
  epochs: 100
  batch_size: 4              # Уменьшите если нехватает памяти GPU
  learning_rate: 0.0001
  optimizer: adam
  scheduler: cosine          # cosine, step, plateau

  # Loss weights
  loss:
    dice_weight: 0.5
    bce_weight: 0.5

  # Augmentations
  augmentation:
    enabled: true
    flip_prob: 0.5
    rotate_prob: 0.5
    brightness_contrast: true
    blur_prob: 0.2

data:
  train_images: data/images/
  train_masks: data/masks/
  train_list: data/train.txt
  val_list: data/val.txt

device: cuda                 # cuda или cpu
save_dir: checkpoints/
tensorboard_dir: runs/
```

### Шаг 4: Обучение модели

```bash
# Запуск обучения
python train.py --config config.yaml

# Продолжение обучения с чекпоинта
python train.py --config config.yaml --resume checkpoints/best_model.pth

# Обучение на CPU (медленно!)
python train.py --config config.yaml --device cpu
```

#### Мониторинг обучения

```bash
# В отдельном терминале
tensorboard --logdir runs/

# Откройте в браузере: http://localhost:6006
```

Метрики обучения:
- **Loss** (Dice + BCE): должен уменьшаться
- **IoU (Intersection over Union)**: должен расти (> 0.7 хорошо)
- **Precision/Recall**: баланс точности
- **F1-score**: общая метрика качества

### Шаг 5: Оценка модели

```bash
# Валидация на тестовом наборе
python train.py --config config.yaml --evaluate --checkpoint checkpoints/best_model.pth

# Визуализация предсказаний
python train.py --config config.yaml --visualize --checkpoint checkpoints/best_model.pth
```

### Шаг 6: Экспорт в ONNX

```bash
# Экспорт обученной модели
python export_onnx.py \
    --checkpoint checkpoints/best_model.pth \
    --output models/unet_cell_detector.onnx \
    --input-size 512 \
    --num-classes 4

# Проверка ONNX модели
python export_onnx.py \
    --checkpoint checkpoints/best_model.pth \
    --output models/unet_cell_detector.onnx \
    --test-image data/images/test.jpg
```

### Шаг 7: Использование в CellAnalyzer

1. Скопируйте .onnx файл в папку приложения:
   ```bash
   cp models/unet_cell_detector.onnx D:/github/cell-analyzer/models/
   ```

2. В CellAnalyzer:
   - Выберите "🧠 Нейросетевая детекция (U-Net)"
   - Нажмите "Обзор..." и выберите `unet_cell_detector.onnx`
   - Настройте параметры (confidence threshold, размеры клеток)
   - Нажмите "Загрузить модель"
   - Начните детекцию!

## Советы по обучению

### Качество данных
- **Минимум 50-100 размеченных изображений** для хорошего результата
- **Разнообразие**: разные условия освещения, масштабы, углы
- **Консистентность**: одинаковый стиль разметки для всех изображений
- **Баланс классов**: примерно равное количество клеток каждого типа

### Аугментации
- **Flip/Rotate**: обязательны (клетки не имеют ориентации)
- **Brightness/Contrast**: полезны для разного освещения
- **Elastic Transform**: помогает с деформациями
- **Cutout/GridMask**: улучшает робастность

### Гиперпараметры
- **Learning Rate**: начните с 1e-4, уменьшите если loss не падает
- **Batch Size**: максимальный, который влезает в GPU
- **Input Size**: 512 (быстро) или 1024 (точнее)
- **Epochs**: 50-100 обычно достаточно

### Overfitting
Признаки:
- Train loss падает, но val loss растет
- IoU на валидации перестает расти

Решения:
- Больше аугментаций
- Dropout в decoder
- Weight decay (L2 regularization)
- Early stopping

### Underfitting
Признаки:
- Оба loss высокие
- IoU < 0.5

Решения:
- Больше эпох обучения
- Более сложная архитектура (ResNet50 вместо ResNet34)
- Меньше регуляризации
- Проверьте качество разметки!

## Troubleshooting

### CUDA Out of Memory
```bash
# Уменьшите batch_size в config.yaml
batch_size: 2  # было 4

# Или уменьшите размер входа
input_size: 512  # было 1024
```

### Низкое качество детекции
1. **Проверьте разметку**: визуализируйте маски
2. **Увеличьте данные**: добавьте больше изображений
3. **Баланс классов**: убедитесь что все типы представлены
4. **Аугментации**: включите больше трансформаций
5. **Fine-tuning**: начните с предобученных весов

### Модель не сходится
1. **Learning rate**: уменьшите в 10 раз
2. **Нормализация**: проверьте mean/std для ImageNet
3. **Loss weights**: отрегулируйте dice/bce баланс
4. **Архитектура**: попробуйте другой encoder

## Примеры команд

```bash
# Полный pipeline
python cvat_utils.py --format coco --input data/annotations.json --output data/masks/
python dataset.py --prepare-split --data-dir data/ --train-ratio 0.8
python train.py --config config.yaml
python export_onnx.py --checkpoint checkpoints/best_model.pth --output models/unet.onnx

# Быстрый тест на 1 изображении
python train.py --config config.yaml --epochs 1 --batch-size 1
python export_onnx.py --checkpoint checkpoints/last.pth --output test.onnx --test-image data/images/test.jpg
```

## Дополнительные ресурсы

- [Segmentation Models PyTorch](https://github.com/qubvel/segmentation_models.pytorch)
- [Albumentations](https://albumentations.ai/)
- [CVAT](https://github.com/opencv/cvat)
- [U-Net Paper](https://arxiv.org/abs/1505.04597)
- [Deep Learning for Cell Image Segmentation](https://www.nature.com/articles/s41592-021-01139-4)

## Контакты и поддержка

Если возникли вопросы или проблемы, создайте issue в репозитории или обратитесь к документации выше.
