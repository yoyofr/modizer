//
//  MiniPlayerImplement.h
//  modizer
//
//  Created by Yohann Magnien on 14/04/2021.
//

#ifndef MiniPlayerImplement_h
#define MiniPlayerImplement_h

-(void)hideMiniPlayer {
    if (miniplayerVC) {
        [miniplayerVC removeFromParentViewController];
        [miniplayerVC.view removeFromSuperview];
        miniplayerVC=nil;
        
        //self.tableView.frame=CGRectMake(0,self.tableView.frame.origin.y,self.tableView.frame.size.width,self.tableView.frame.size.height+48);
        //[tableView reloadData];
    }
}
-(void)updateMiniPlayer {
    miniplayerVC.coverImg=(detailViewController.cover_img?detailViewController.cover_img:detailViewController.default_cover);
    [miniplayerVC refreshCoverLabels];
}

- (void)showMiniPlayer {
    [self hideMiniPlayer];
    
    miniplayerVC = [[MiniPlayerVC alloc] init];
    //set new title
    miniplayerVC.title = @"Mini Player";
    miniplayerVC.detailVC=detailViewController;
    miniplayerVC.coverImg=(detailViewController.cover_img?detailViewController.cover_img:detailViewController.default_cover);
    miniplayerVC.view.translatesAutoresizingMaskIntoConstraints=false;
    
    [self addChildViewController:miniplayerVC];
    [self.view addSubview:miniplayerVC.view];
    
    UIView *miniplayerVCview=miniplayerVC.view;
    NSDictionary *views = NSDictionaryOfVariableBindings(miniplayerVCview);
    // height constraint
    [miniplayerVCview addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[miniplayerVCview(48)]" options:0 metrics:nil views:views]];
    //positioning constraints
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:miniplayerVC.view attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeWidth relatedBy:NSLayoutRelationEqual toItem:miniplayerVC.view attribute:NSLayoutAttributeWidth multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationEqual toItem:miniplayerVC.view attribute:NSLayoutAttributeBottom multiplier:1.0 constant:0]];
    
    //self.tableView.frame=CGRectMake(0,self.tableView.frame.origin.y,self.tableView.frame.size.width,self.tableView.frame.size.height-48);
    //[tableView reloadData];
}

-(void) goCurrentPlaylist {
    if (detailViewController.mPlaylist_size) {
        t_playlist* temp_playlist=(t_playlist*)malloc(sizeof(t_playlist));
        memset(temp_playlist,0,sizeof(t_playlist));
        
        if (detailViewController.mPlaylist_size) { //display current queue
            for (int i=0;i<detailViewController.mPlaylist_size;i++) {
                temp_playlist->entries[i].label=[[NSString alloc] initWithString:detailViewController.mPlaylist[i].mPlaylistFilename];
                temp_playlist->entries[i].fullpath=[[NSString alloc ] initWithString:detailViewController.mPlaylist[i].mPlaylistFilepath];
                
                temp_playlist->entries[i].ratings=detailViewController.mPlaylist[i].mPlaylistRating;
                temp_playlist->entries[i].playcounts=-1;
            }
            temp_playlist->nb_entries=detailViewController.mPlaylist_size;
            temp_playlist->playlist_name=@"Now playing";
            temp_playlist->playlist_id=nil;
            
            RootViewControllerPlaylist *nowplayingPL = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            //set new title
            nowplayingPL.title = temp_playlist->playlist_name;
            ((RootViewControllerPlaylist*)nowplayingPL)->show_playlist=1;
            
            // Set new directory
            ((RootViewControllerPlaylist*)nowplayingPL)->browse_depth = 1;
            ((RootViewControllerPlaylist*)nowplayingPL)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)nowplayingPL)->playlist=temp_playlist;
            ((RootViewControllerPlaylist*)nowplayingPL)->mFreePlaylist=1;
            ((RootViewControllerPlaylist*)nowplayingPL)->mDetailPlayerMode=1;
            ((RootViewControllerPlaylist*)nowplayingPL)->integrated_playlist=INTEGRATED_PLAYLIST_NOWPLAYING;
            ((RootViewControllerPlaylist*)nowplayingPL)->currentPlayedEntry=detailViewController.mPlaylist_pos+1;
            
            // And push the window
            [self.navigationController pushViewController:nowplayingPL animated:YES];
        }
    }
}



#endif /* MiniPlayerImplement_h */
