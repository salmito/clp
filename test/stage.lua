local lstage=require'lstage'

print(lstage.default:size())

lstage.stage(function() print('run',lstage.default:size()) end,1):push()
lstage.event.sleep(1)
