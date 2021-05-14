//
//  ImagesCache.m
//
//  Created by Mihael Isaev on 19.01.13.
//  Copyright (c) 2013 Mihael Isaev, Russia, Samara. All rights reserved.
//

#import "ImagesCache.h"

#define directoryInCache @"imagesCache"

@implementation ImagesCache

-(id)init
{
    //Initialize nscache objects
    //Инициализируем кэш-объекты
    imagesReal = [[NSCache alloc] init];
    imagesScaled = [[NSCache alloc] init];
    imagesScaledRetina = [[NSCache alloc] init];
    [self createDirectory:directoryInCache atFilePath:[self cacheDirectory]];
    return self;
}

-(UIImage*)getImageWithURL:(NSString*)url
                    prefix:(NSString*)prefix
                      size:(CGSize)size
            forUIImageView:(UIImageView*)uiimageview
{
    //If retina detected then remake size to *2
    //Если ретина экран тогда умножаем size на два
    if([self isRetina])
        size = CGSizeMake(size.width*2, size.height*2);
    //Create array with parts of URL separated by / symbol for get image name with extension
    //Создаем массив с частями URL разделенные знаком / для получения имени картинки с расширением
    NSArray *imageURLParts = [url componentsSeparatedByString:@"/"];
    //Getting last object from imageURLParts with image name and extension separated by dot
    //Получаем последний объект в массиве imageURLParts с именем и расширение картинки разделенные точкой
    NSString *lastObjectInImageURLParts = [imageURLParts lastObject];
    //Create array with parts of image name separated by . symbol for get name and extension
    //Создаем массив с частями названия картинки разделенные знаком . для получения отдельно имени и расширения картинки
    NSArray *imageNameParts = [lastObjectInImageURLParts componentsSeparatedByString:@"."];
    //Create strings with image name and image extension
    //Создаем строки содержащие название файла и расширение файла
    NSString *imageName = [NSString stringWithFormat:@"%@_%@", prefix, [imageNameParts objectAtIndex:0]];
    NSString *imageExtension = [imageNameParts objectAtIndex:1];
    //Create link to cache object by checking retina display or not
    //Создаем ссылку на объект кэша выбирая нужный взависимости от того retina дисплей или нет
    NSCache *cache = ([self isRetina]) ? imagesScaledRetina : imagesScaled;
    //Create uiimage object
    //Создаем объект uiimage
    UIImage *image = [[UIImage alloc] init];
    //Try to get image from cache by image name without extension
    //Пробуем получить картинку из кэша по ее имени без расширения
    image = [cache objectForKey:imageName];
    //Check if image is exists in cache and return it
    //Проверяем если картинка есть в кэше, тогда возвращаем ее
    if(image)
        return image;
    //So let's try to open image from saved file on disk (if he is exists)
    //Ну что ж пробуем открыть картинку из сохраненного файла на диске (если он существует)
    image = [self getImageWithName:[self makeNameWithPrefix:prefix name:imageName] type:ICTypeScaled extension:imageExtension];
    //Check if image is exists on disk and return it
    //Проверяем получили ли мы картинку с диска и возвращаем ее
    if(image)
        return image;
    //So, we haven't image in cache and disk, and we need to download it from the network
    //Что ж у нас нет картинки ни в кэше ни на диске, значит мы должны скачать ее из сети
    //Before downloading setting in uiimage temporary placeholder.png image with loading text
    //Перед скачиванием задаем в uiimage временную картинку placeholder.png с текстом loading
    uiimageview.image = [UIImage imageNamed:ICPlaceholderFile];
    //Create background thread for download and scale image without blocking our UI
    //Создаем фоновый поток для скачивания и сжатия картинки не блокируя интерфейс нашей программы
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        //Try to get real image from cache by image name without extension
        //Пробуем получить оригинальную картинку из кэша по ее имени без расширения
        UIImage *realImage = [imagesReal objectForKey:imageName];
        //Check, and if real image is not exists in cache let's try to open real image from saved file on disk (if he is exists)
        //Проверяем, если оригинальной картинки нет в кэше, тогда пытаемся открыть оригинальную картинку из сохраненного файла на диске (если он существует)
        if(!realImage)
            realImage = [self getImageWithName:imageName type:ICTypeReal extension:imageExtension];
        //Check, if original image still not exists we start getting it from the network
        //Проверяем, если оригинальной картинки все еще нет мы начинаем загрузку ее из сети
        if(!realImage)
            //We'll be careful and use try/catch for downloading
            //Мы будем осторожны и используем try/catch при загрузке
            @try {
                //Making NSURL object with our url string
                //Создаем NSURL объект с нашей строкой url
                //NSLog(@"image url: %@", url);
                NSURL *nsurl = [NSURL URLWithString:url];
                //Download file to NSData object with using nsurl
                //Загружаем файл в NSData объект с использованием nsurl
                NSData *nsdata = [NSData dataWithContentsOfURL:nsurl];
                //Setting real image with getted nsdata
                //Задаем оригинальную картинку с данными из nsdata
                realImage = [[UIImage alloc] initWithData:nsdata];
                //Save downloaded real image to file on disk
                //Сохраняем загруженную оригинальную картинку в файл на диск
                [self saveImage:realImage toFileWithName:[self makeNameWithPrefix:prefix name:imageName] type:ICTypeReal extension:imageExtension];
            }
        @catch (NSException *exception) {
            NSLog(@"Error downloading image with name: %@", imageName);
        }
        //Check if real image is exists
        //Проверяем что оригинальная картинка теперь у нас есть
        if(realImage)
        {
            //Save original image to imagesReal cache
            //Сохраняем оригинальную картинку в кэш imagesReal
            if(realImage)
                [imagesReal setObject:realImage forKey:imageName];
            //Create scaled image object from real image
            //Создаем объект со сжатой картинкой из оригинальной картинки
            UIImage *scaledImage = [self scaleImage:realImage toSize:size];
            //Save scaled image to scaled images cache
            //Сохраняем сжатую картинку в кэш сжатых картинок
            if(scaledImage)
                [cache setObject:scaledImage forKey:imageName];
            //Save scaled real image to file on disk
            //Сохраняем сжатую оригинальную картинку в файл на диск
            [self saveImage:scaledImage toFileWithName:[self makeNameWithPrefix:prefix name:imageName] type:ICTypeScaled extension:imageExtension];
            //In main thread we set scaled image object to uiimage
            //В главном потоке мы зададим сжатую картинку в uiimage
            dispatch_async(dispatch_get_main_queue(), ^{
                //We'll be careful and use try/catch
                //Мы будем осторожны и используем try/catch
                @try {
                    //Check for uiimage is not released and set scaled image to it
                    //Проверим не очистился ли еще объект uiimage и задаем в него сжатую картинку
                    if(uiimageview && scaledImage)
                        uiimageview.image = scaledImage;
                }
                @catch (NSException *exception) {
                    NSLog(@"Error setting uiimage with name: %@", imageName);
                }
            });
        }
    });
    //Return uiimage
    //Возвращаем uiimage
    return uiimageview.image;
}

//Helper function for make image filename
//Вспомогательная функция для создания имени картинки
-(NSString*)makeNameWithPrefix:(NSString*)prefix
                          name:(NSString*)name
{
    NSString *retina = ([self isRetina]) ? @"retina" : @"normal";
    return [NSString stringWithFormat:@"%@_%@_%@", prefix, name, retina];
}

//Helper function for detect retina screen
//Вспомогательная функция для определения retina дисплея
-(BOOL)isRetina
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        CGFloat scale = [[UIScreen mainScreen] scale];
        if (scale > 1.0)
            //iPad retina display
            return YES;
        else
            //iPad not retina display
            return NO;
    }
    else
        if ([UIScreen instancesRespondToSelector:@selector(scale)])
        {
            CGFloat scale = [[UIScreen mainScreen] scale];
            if (scale > 1.0)
                if([[ UIScreen mainScreen ] bounds ].size.height == 568)
                    //iphone 5 retina display
                    return YES;
                else
                    //iphone retina display
                    return YES;
                else
                    //iphone not retina display
                    return NO;
        }
        else
            //other not retina display
            return NO;
}


//Helper function for resize image
//Вспомогательная функция для изменения размера картинки
-(UIImage*)scaleImage:(UIImage*)image
               toSize:(CGSize)newSize
{
    UIGraphicsBeginImageContext( newSize );
    //[image drawInRect:CGRectMake(0,0,newSize.width,newSize.height)];
    
    CGRect recto=CGRectMake(0,0,newSize.width,newSize.height);
    if (newSize.width <= CGSizeZero.width && newSize.height <= CGSizeZero.height ) {
        [image drawInRect:recto];
    }

    float widthRatio = newSize.width  / image.size.width;
    float heightRatio   = newSize.height / image.size.height;
    float scalingFactor = fmin(widthRatio, heightRatio);
    CGSize newSizeImg = CGSizeMake(image.size.width  * scalingFactor, image.size.height * scalingFactor);

    CGPoint origin = CGPointMake((newSize.width-newSizeImg.width)/2,(newSize.height - newSizeImg.height) / 2);

    [image drawInRect:CGRectMake(origin.x, origin.y, newSizeImg.width, newSizeImg.height)];
    UIImage* scaledImage = UIGraphicsGetImageFromCurrentImageContext();
    
    [scaledImage drawInRect:recto];
    
    
    
    UIImage* newImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return newImage;
}

//Helper function for saving image to file on disk
//Вспомогательная функция для сохранения картинки в файл на диск
-(void)saveImage:(UIImage*)image
  toFileWithName:(NSString*)name
            type:(NSString*)type
       extension:(NSString*)extension
{
    if(!image)
    {
        NSLog(@"incoming image is nil");
        return;
    }
    //String with link to file in folder
    //Строка со ссылкой на файл в папке
    NSString *fileWithPathAndExtension = [[self cacheDirectory] stringByAppendingFormat:@"/%@/%@_%@.%@", directoryInCache, name, type, extension];
    //Create NSData object with image
    //Создаем объект NSData с картинкой
    NSData *data = ([extension isEqualToString:@"png"]) ? UIImagePNGRepresentation(image) : UIImageJPEGRepresentation(image, 1.0);
    //Writing file to disk
    //Запись файла на диск
    [data writeToFile:fileWithPathAndExtension atomically:YES];
}


//Helper function for getting image from file on disk
//Вспомогательная функция для открытия картинки из файла на диске
- (UIImage*)getImageWithName:(NSString*)name
                        type:(NSString*)type
                   extension:(NSString*)extension
{
    //String with link to file in folder
    //Строка со ссылкой на файл в папке
    NSString *fileWithPathAndExtension = [[self cacheDirectory] stringByAppendingFormat:@"/%@/%@_%@.%@", directoryInCache, name, type, extension];
    return [UIImage imageWithContentsOfFile:fileWithPathAndExtension];
}

-(NSString*)cacheDirectory
{
    NSString *cachePath = NSTemporaryDirectory();
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
    if(paths.count>0)
        cachePath = [paths objectAtIndex:0];
    return cachePath;
}

-(void)createDirectory:(NSString *)directoryName atFilePath:(NSString *)filePath
{
    NSString *filePathAndDirectory = [filePath stringByAppendingPathComponent:directoryName];
    NSError *error = nil;
    if (![[NSFileManager defaultManager] createDirectoryAtPath:filePathAndDirectory
                                   withIntermediateDirectories:NO
                                                    attributes:nil
                                                         error:&error])
        NSLog(@"Create directory error: %@", error);
}

@end
