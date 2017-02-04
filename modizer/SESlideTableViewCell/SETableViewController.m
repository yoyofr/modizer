//
//  SETableViewController.m
//  slidetableviewcell
//
//  Created by letdown on 2014/07/09.
//  Copyright (c) 2014 Space Elephant. All rights reserved.
//

#import "SETableViewController.h"

#import "SESlideTableViewCell.h"

enum {
	CellIndexRightButtons,
	CellIndexRightButtonsAndDisclosureIndicator,
	CellIndexRightLongButtons,
	CellIndexThreeRightButtons,
	CellIndexRightImageButton,
	CellIndexRightImageTextButton,
	CellIndexLeftRightButtons,
	CellIndexLeftButtons,
	CellIndexBackgroundColor,
	CellIndexContentUpdate,
	CellIndexSlideElasticity,
	
	CellIndexCount,
};

@interface SETableViewController () <SESlideTableViewCellDelegate> {
	UILocalizedIndexedCollation* m_collation;
	BOOL m_rightButtonDisabled;
	BOOL m_leftButtonDisabled;
	int m_value;
}

@end

@implementation SETableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
	
	{
		UIBarButtonItem* button = [[UIBarButtonItem alloc] initWithTitle:@"Index" style:UIBarButtonItemStylePlain target:self action:@selector(indexButtonDidTap:)];
		self.navigationItem.rightBarButtonItem = button;
	}
	{
		UISegmentedControl* segmentedControl = [[UISegmentedControl alloc] initWithItems:@[@"L", @"R"]];
		segmentedControl.momentary = YES;
		[segmentedControl addTarget:self action:@selector(toggleLeftAndRightButtons:) forControlEvents:UIControlEventValueChanged];
		UIBarButtonItem* button = [[UIBarButtonItem alloc] initWithCustomView:segmentedControl];
		self.navigationItem.leftBarButtonItem = button;
	}
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	if (m_collation) {
		return m_collation.sectionIndexTitles.count;
	}
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return CellIndexCount;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
	if (m_collation) {
		return [m_collation sectionForSectionIndexTitleAtIndex:index];
	}
	return index;
}

- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
	if (m_collation) {
		return m_collation.sectionIndexTitles;
	}
	return nil;
}

- (NSString*)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	if (m_collation) {
		return m_collation.sectionIndexTitles[section];
	}
	return nil;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	switch (indexPath.row) {
		default:
		case CellIndexRightButtons: {
			NSString* const CELL_ID = @"Cell0";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;

				cell.textLabel.text = @"Cell with Right Buttons";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexRightButtonsAndDisclosureIndicator: {
			NSString* const CELL_ID = @"Cell1";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with Right Buttons and Disclosure Indicator";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexRightLongButtons: {
			NSString* const CELL_ID = @"Cell2";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with Right Buttons";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"World with Love" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0] font:[UIFont boldSystemFontOfSize:24]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexThreeRightButtons: {
			NSString* const CELL_ID = @"Cell3";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with three Right Buttons";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"World" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"Again" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:120.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexRightImageButton: {
			NSString* const CELL_ID = @"Cell4";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with a Right Image Button";
				[cell addRightButtonWithImage:[UIImage imageNamed:@"image-cloud"] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexRightImageTextButton: {
			NSString* const CELL_ID = @"CellIndexRightImageTextButton";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with a Right Image Text Button";
				[cell addRightButtonWithImage:[UIImage imageNamed:@"image-cloud-small"] text:@"Hello" textColor:[UIColor whiteColor] font:nil backgroundColor:[UIColor colorWithHue:0.0/360.0f saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexLeftRightButtons: {
			NSString* const CELL_ID = @"Cell5";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with Left and Right Buttons";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"Right World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addLeftButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addLeftButtonWithText:@"Left World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexLeftButtons: {
			NSString* const CELL_ID = @"Cell6";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				cell.textLabel.text = @"Cell with Left Buttons";
				[cell addLeftButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addLeftButtonWithText:@"World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}	break;
		case CellIndexBackgroundColor: {
			NSString* const CELL_ID = @"Cell7";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				UIColor* bgColor = [UIColor colorWithHue:210.0/360 saturation:0.1 brightness:0.9 alpha:1.0];
				cell.backgroundColor = bgColor;
				cell.slideBackgroundColor = bgColor;
				cell.indicatorColor = [UIColor colorWithHue:12/360.0f saturation:0.8f brightness:0.9f alpha:1.0f];
				
				cell.textLabel.text = @"Cell with Background Color";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"World!!" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}
		case CellIndexContentUpdate: {
			NSString* const CELL_ID = @"Cell8";
			
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.delegate = self;
				
				[cell addRightButtonWithText:@"-" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addRightButtonWithText:@"+" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			[self configureCell:cell atIndex:CellIndexContentUpdate];

			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}
		case CellIndexSlideElasticity: {
			NSString* const CELL_ID = @"CellIndexSlideElasticity";
			SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView dequeueReusableCellWithIdentifier:CELL_ID];
			if (cell == nil) {
				cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CELL_ID];
				cell.selectionStyle = UITableViewCellSelectionStyleNone;
				cell.slideElasticity = SESlideTableViewCellSlideElasticityHard;
				cell.delegate = self;
				cell.textLabel.text = @"Cell with Hard Elasticity";
				[cell addRightButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:180.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
				[cell addLeftButtonWithText:@"Hello" textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithHue:0.0/360.0 saturation:0.8 brightness:0.9 alpha:1.0]];
			}
			cell.showsLeftSlideIndicator = ! m_leftButtonDisabled;
			cell.showsRightSlideIndicator = ! m_rightButtonDisabled;
			return cell;
		}
	}
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	SESlideTableViewCell* cell = (SESlideTableViewCell*)[tableView cellForRowAtIndexPath:indexPath];
	switch (cell.slideState) {
		case SESlideTableViewCellSlideStateCenter:
			[cell setSlideState:SESlideTableViewCellSlideStateRight animated:YES];
			break;
		default:
			[cell setSlideState:SESlideTableViewCellSlideStateCenter animated:YES];
			break;
	}
}

#pragma mark -

- (void)indexButtonDidTap:(id)sender {
	if (m_collation) {
		m_collation = nil;
	} else {
		m_collation = [UILocalizedIndexedCollation currentCollation];
	}
	[self.tableView reloadData];
}

- (void)toggleLeftAndRightButtons:(UISegmentedControl*)sender {
	switch (sender.selectedSegmentIndex) {
	case 0: {
		m_leftButtonDisabled = ! m_leftButtonDisabled;
		[self.tableView reloadData];
		UIAlertView* alertView = [[UIAlertView alloc] initWithTitle:@"Left Button" message:(m_leftButtonDisabled) ? @"Disabled": @"Enabled" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
		[alertView show];
	}	break;
	case 1:
		m_rightButtonDisabled = ! m_rightButtonDisabled;
		[self.tableView reloadData];
		UIAlertView* alertView = [[UIAlertView alloc] initWithTitle:@"Right Button" message:(m_leftButtonDisabled) ? @"Disabled": @"Enabled" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
		[alertView show];
		break;
	}
}

- (void)configureCell:(UITableViewCell*)cell atIndex:(NSUInteger)index {
	if (index == CellIndexContentUpdate) {
		cell.textLabel.text = [NSString stringWithFormat:@"Cell with value %d", m_value];
	}
}

#pragma mark - 

- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerLeftButton:(NSInteger)buttonIndex {
	NSLog(@"left button triggered:%d", (int)buttonIndex);
}

- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerRightButton:(NSInteger)buttonIndex {
	NSLog(@"right button triggered:%d", (int)buttonIndex);
	NSIndexPath* indexPath = [self.tableView indexPathForCell:cell];
	if (indexPath.row == CellIndexContentUpdate) {
		if (buttonIndex == 0) {
			m_value--;
		} else {
			m_value++;
		}
		[self configureCell:cell atIndex:CellIndexContentUpdate];
		[cell updateContentViewSnapshot];
	}
}

- (BOOL)slideTableViewCell:(SESlideTableViewCell*)cell canSlideToState:(SESlideTableViewCellSlideState)slideState {
	switch (slideState) {
	case SESlideTableViewCellSlideStateLeft:
		return !m_leftButtonDisabled;
	case SESlideTableViewCellSlideStateRight:
		return !m_rightButtonDisabled;
	default:
		return YES;
	}
}

- (void)slideTableViewCell:(SESlideTableViewCell *)cell willSlideToState:(SESlideTableViewCellSlideState)slideState {
	NSIndexPath* indexPath = [self.tableView indexPathForCell:cell];
	if (indexPath) {
		NSLog(@"cell at %d - %d will slide to state %d", (int)indexPath.section, (int)indexPath.row, (int)slideState);
	}
}

- (void)slideTableViewCell:(SESlideTableViewCell *)cell didSlideToState:(SESlideTableViewCellSlideState)slideState {
	NSIndexPath* indexPath = [self.tableView indexPathForCell:cell];
	if (indexPath) {
		NSLog(@"cell at %d - %d did slide to state %d", (int)indexPath.section, (int)indexPath.row, (int)slideState);
	}
}

- (void)slideTableViewCell:(SESlideTableViewCell *)cell wilShowButtonsOfSide:(SESlideTableViewCellSide)side {
	NSIndexPath* indexPath = [self.tableView indexPathForCell:cell];
	if (indexPath) {
		NSLog(@"cell at %d - %d will show buttons of side %d", (int)indexPath.section, (int)indexPath.row, (int)side);
	}
}

@end
