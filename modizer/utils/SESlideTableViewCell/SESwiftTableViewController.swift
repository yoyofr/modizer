//
//  SESwiftTableViewController.swift
//  slidetableviewcell
//
//  Created by letdown on 2014/07/15.
//  Copyright (c) 2014 Space Elephant. All rights reserved.
//

import UIKit

enum CellIndex : Int {
	case rightButtons
	case rightButtonsAndDisclosureIndicator
	case rightLongButtons
	case threeRightButtons
	case rightImageButton
	case rightImageTextButton
	case leftRightButtons
	case leftButtons
	case backgroundColor
	case contentUpdate
	case slideElasticity
	
	case count
}

class SESwiftTableViewController: UITableViewController, SESlideTableViewCellDelegate {
	
	var collation: UILocalizedIndexedCollation?
	var rightButtonDisabled: Bool = false
	var leftButtonDisabled: Bool = false
	var value: Int = 0
	
    override func viewDidLoad() {
        super.viewDidLoad()
		
		let button = UIBarButtonItem(title: "Index", style: .plain, target:self, action: #selector(SESwiftTableViewController.indexButtonDidTap(_:)))
		navigationItem.rightBarButtonItem = button
		
		let segmentedControl = UISegmentedControl(items: ["L" as NSString, "R" as NSString])
		segmentedControl.isMomentary = true
		segmentedControl.addTarget(self, action: #selector(SESwiftTableViewController.toggleLeftAndRightButtons(_:)), for: .valueChanged)
		self.navigationItem.leftBarButtonItem = UIBarButtonItem(customView: segmentedControl)
	}

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }

    //MARK - Table view data source

    override func numberOfSections(in tableView: UITableView) -> Int {
		if collation != nil {
			return collation!.sectionIndexTitles.count
		} else {
			return 1
		}

	}

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return CellIndex.count.rawValue
	}
	
	override func tableView(_ tableView: UITableView, sectionForSectionIndexTitle title: String, at index: Int) -> Int {
		if collation != nil {
			return collation!.section(forSectionIndexTitle: index)
		}
		return index
	}
	
	override func sectionIndexTitles(for tableView: UITableView) -> [String]? {
		return collation?.sectionIndexTitles
	}
	
	override func tableView(_ tableView: UITableView, titleForHeaderInSection section: Int) -> String? {
		if collation != nil {
			return collation!.sectionTitles[section]
		}
		return nil
	}
	
    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		
		let cellIndex = CellIndex(rawValue: (indexPath as NSIndexPath).row)
		switch cellIndex! {
		case .rightButtons:
			let CELL_ID = "Cell0"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with Right Buttons"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .rightButtonsAndDisclosureIndicator:
			let CELL_ID = "Cell1"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with Right Buttons and Disclosure Indicator"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.accessoryType = .disclosureIndicator
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .rightLongButtons:
			let CELL_ID = "Cell2"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with Right Buttons"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "World with Love", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0), font:UIFont.boldSystemFont(ofSize: 24))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .threeRightButtons:
			let CELL_ID = "Cell3"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with three Right Buttons"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "World", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "Again", textColor: UIColor.white, backgroundColor: UIColor(hue: 120.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .rightImageButton:
			let CELL_ID = "Cell4"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with a Right Image Button"
				cell!.addRightButton(with: UIImage(named: "image-cloud"), backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .rightImageTextButton:
			let CELL_ID = "CellRightImageTextButton"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with a Right Image Text Button"
				cell!.addRightButton(with: UIImage(named: "image-cloud-small"), text: "Hello", textColor: UIColor.white, font: nil, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .leftRightButtons:
			let CELL_ID = "Cell5"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with Left and Right Buttons"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "Right World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addLeftButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addLeftButton(withText: "Left World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .leftButtons:
			let CELL_ID = "Cell6"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				
				cell!.textLabel!.text = "Cell with Left Buttons"
				cell!.addLeftButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addLeftButton(withText: "World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.accessoryType = .disclosureIndicator
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .backgroundColor:
			let CELL_ID = "Cell7"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				let bgColor = UIColor(hue: 210/360.0, saturation: 0.1, brightness: 0.9, alpha: 1.0)
				cell!.backgroundColor = bgColor
				cell!.slideBackgroundColor = bgColor
				cell!.indicatorColor = UIColor(hue: 12/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0)
				
				cell!.textLabel!.text = "Cell with Background Color"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "World!!", textColor: UIColor.white, backgroundColor: UIColor(hue: 180/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .contentUpdate:
			let CELL_ID = "Cell8"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.delegate = self
				cell!.addRightButton(withText: "-", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addRightButton(withText: "+", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			configureCell(cell, atIndex: (indexPath as NSIndexPath).row)
			
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		case .slideElasticity:
			let CELL_ID = "CellSlideElasticity"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
				cell!.slideElasticity = .hard
				cell!.delegate = self
				cell!.textLabel!.text = "Cell with Hard Elasticity"
				cell!.addRightButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 180.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
				cell!.addLeftButton(withText: "Hello", textColor: UIColor.white, backgroundColor: UIColor(hue: 0.0/360.0, saturation: 0.8, brightness: 0.9, alpha: 1.0))
			}
			
			cell!.showsLeftSlideIndicator = !leftButtonDisabled
			cell!.showsRightSlideIndicator = !rightButtonDisabled
			return cell!
		default:
			let CELL_ID = "Cell"
			var cell = tableView.dequeueReusableCell(withIdentifier: CELL_ID) as? SESlideTableViewCell
			if cell == nil {
				cell = SESlideTableViewCell(style: .default, reuseIdentifier: CELL_ID)
				cell!.selectionStyle = .none
			}
			return cell!
		}
    }

	override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		let cell = tableView.cellForRow(at: indexPath) as! SESlideTableViewCell!
		switch cell!.slideState {
		case .center:
			cell?.setSlideState(.right, animated:true)
		default:
			cell?.setSlideState(.center, animated:true)
		}
	}

	// #pragma mark -
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, didTriggerLeftButton buttonIndex: NSInteger) {
		let indexPath = tableView.indexPath(for: cell)
		print("left button \(buttonIndex) tapped in cell \((indexPath as NSIndexPath?)?.section) - \((indexPath as NSIndexPath?)?.row)")
	}
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, didTriggerRightButton buttonIndex: NSInteger) {
		let indexPath = tableView.indexPath(for: cell)
		print("right button \(buttonIndex) tapped in cell \((indexPath as NSIndexPath?)?.section) - \((indexPath as NSIndexPath?)?.row)")
		if (indexPath as NSIndexPath?)?.row == CellIndex.contentUpdate.rawValue {
			switch buttonIndex {
			case 0:
				value -= 1
			case 1:
				value += 1
			default:
				break
			}
			self.configureCell(cell, atIndex: (indexPath! as NSIndexPath).row)
			cell.updateContentViewSnapshot()
		}
	}
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, canSlideTo slideState: SESlideTableViewCellSlideState) -> Bool {
		switch (slideState) {
		case .left:
			return !leftButtonDisabled
		case .right:
			return !rightButtonDisabled
		default:
			break
		}
		return true
	}
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, willSlideTo slideState: SESlideTableViewCellSlideState) {
		if let indexPath = tableView.indexPath(for: cell) {
			print("cell at \((indexPath as NSIndexPath).section) - \((indexPath as NSIndexPath).row) will slide to state \(slideState.rawValue)")
		}
	}
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, didSlideTo slideState: SESlideTableViewCellSlideState) {
		if let indexPath = tableView.indexPath(for: cell) {
			print("cell at \((indexPath as NSIndexPath).section) - \((indexPath as NSIndexPath).row) did slide to state \(slideState.rawValue)")
		}
	}
	
	func slideTableViewCell(_ cell: SESlideTableViewCell!, wilShowButtonsOf side: SESlideTableViewCellSide) {
		if let indexPath = tableView.indexPath(for: cell) {
			print("cell at \((indexPath as NSIndexPath).section) - \((indexPath as NSIndexPath).row) will show buttons of side \(side.rawValue)")
		}
	}
	
	func configureCell(_ cell: SESlideTableViewCell!, atIndex index:Int) {
		if index == CellIndex.contentUpdate.rawValue {
			cell.textLabel?.text = "Cell with value \(value)"
		}
	}
	// #pragma mark -
	
	func indexButtonDidTap(_ sender: AnyObject!) {
		if collation != nil {
			collation = nil
		} else {
			collation = UILocalizedIndexedCollation.current()
		}
		tableView.reloadData()
	}
	
	func toggleLeftAndRightButtons(_ sender: UISegmentedControl!) {
		switch (sender.selectedSegmentIndex) {
		case 0:
			leftButtonDisabled = !leftButtonDisabled
			self.tableView.reloadData()
			let message = leftButtonDisabled ? "Disabled" : "Enabled"
			let alertView = UIAlertView(title: "Left Button", message: message, delegate: nil, cancelButtonTitle: "OK")
			alertView.show()
		case 1:
			rightButtonDisabled = !rightButtonDisabled
			self.tableView.reloadData()
			let message = rightButtonDisabled ? "Disabled" : "Enabled"
			let alertView = UIAlertView(title: "Right Button", message: message, delegate: nil, cancelButtonTitle: "OK")
			alertView.show()
		default:
			break
		}
	}
}
